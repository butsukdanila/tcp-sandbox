#include "x-server.h"
#include "x-server-api.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-inet.h"
#include "x-list.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
  WST_RECV = 0,
  WST_SEND = 1,
} worker_state_t;

typedef struct {
  pollfd_t       *pfd;
  char           *addr;
  in_port_t       port;
  worker_state_t  state;
  server_xchg_t   xchg;
  size_t          msgc;
  list_t          node;
} worker_t;

#define worker_for_each_safe(_data, _temp, _head) \
  list_for_each_data_safe(_data, _temp, _head, node)

#define logi_worker(_a, _f, ...) \
  logi("worker[%d] " _f, (_a)->port, ##__VA_ARGS__)

#define loge_worker(_a, _f, ...) \
  loge("worker[%d] " _f, (_a)->port, ##__VA_ARGS__)

static worker_t *
worker_new(pollfd_t *pfd, char *addr, in_port_t port) {
  worker_t *worker = malloc(sizeof(*worker));
  worker->pfd = pfd;
  worker->addr = addr;
  worker->port = port;
  worker->state = WST_RECV;
  worker->xchg = server_xchg_init(worker->pfd);
  server_xchg_recv_prep(&worker->xchg);
  worker->msgc = 0;
  return worker;
}

static void
worker_free(worker_t *worker) {
  if (!worker) return;
  free(worker->addr);
  server_xchg_disp(&worker->xchg);
  free(worker);
}

static void
worker_op_ping(worker_t *worker __unused, server_frame_t *rsp) {
  logi_worker(worker, "PING");
  static const char pong[] = "pong";
  server_frame_body_set(rsp, pong, sizeof(pong));
}

static void
worker_op_message(worker_t *worker, server_frame_t *rsp __unused) {
  logi_worker(worker, "MESSAGE[%03zu]: %s",
    worker->msgc++,
    (char *)worker->xchg.frame.body);
}

static server_frame_t
worker_op_callback(worker_t *worker) {
  const server_frame_t *req = &worker->xchg.frame;
  server_frame_t       *rsp = &server_frame_rsp();
  switch (req->head.req.opcode) {
    case SREQ_OP_PING:    worker_op_ping(worker, rsp);    break;
    case SREQ_OP_MESSAGE: worker_op_message(worker, rsp); break;
    default: {
      server_error_any(rsp, SERR_NOP, "unknown: %u", req->head.req.opcode);
      break;
    }
  }
  return *rsp;
}

static int
worker_xchg(worker_t *worker) {
  if (worker->state == WST_RECV) {
    int rc = server_xchg_recv(&worker->xchg);
    if (rc == SUCCESS) {
      server_frame_t rsp = worker_op_callback(worker);
      server_xchg_disp(&worker->xchg);
      worker->xchg.frame = rsp;
      server_xchg_send_prep(&worker->xchg);
      worker->state = WST_SEND;
    }
    // logi_worker(worker, "[RECV] %d", rc);
    return rc;
  }
  if (worker->state == WST_SEND) {
    int rc = server_xchg_send(&worker->xchg);
    if (rc == SUCCESS) {
      server_frame_head_zero(&worker->xchg.frame);
      server_xchg_recv_prep(&worker->xchg);
      worker->state = WST_RECV;
    }
    // logi_worker(worker, "[SEND] %d", rc);
    return rc;
  }
  loge_worker(worker, "something went horribly wrong");
  return FAILURE;
}

struct server {
  pollfd_t      *pfd;
  pollfd_pool_t *pfdpool;
  list_t         workers;
};

static int
server_socket(const char *address, const char *port, const int backlog) {
  addrinfo_t *ai_hint = &(addrinfo_t) {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  addrinfo_t *ai = null;
  int rc = getaddrinfo(address, port, ai_hint, &ai);
  if (rc < 0) {
    loge("server socket getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    return -1;
  }
  int fd = -1;
  for (addrinfo_t *p = ai; p; p = p->ai_next) {
    fd = socket(ai->ai_family, ai->ai_socktype | SOCK_NONBLOCK, ai->ai_protocol);
    if (fd < 0) {
      loge_errno("server socket failure");
      continue;
    }
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
      loge_errno("server socket setsockopt failure");
      goto __probe_next;
    }
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
      loge_errno("server socket bind failure");
      goto __probe_next;
    }
    if (listen(fd, backlog) < 0) {
      loge_errno("server socket listen failure");
      goto __probe_next;
    }
    char      *in_addr = null;
    in_port_t  in_port = 0;
    if (getsocknamestr(fd, &in_addr, &in_port) < 0) {
      loge_errno("server socket getpeername failure");
      goto __probe_next;
    }
    logi("server ready: %s:%u", in_addr, ntohs(in_port));
    free(in_addr);
    break;

  __probe_next:
    close(fd);
    fd = -1;
  }
  freeaddrinfo(ai);
  return fd;
}

static int
server_worker_init(server_t *server) {
  if (!(server->pfd->revents & POLLIN)) {
    return IGNORED;
  }
  pollfd_t *pfd = pollfd_pool_borrow(server->pfdpool);
  if (!pfd) {
    loge("worker failed to borrow pollfd");
    return FAILURE;
  }
  if ((pfd->fd = accept(server->pfd->fd, null, null)) < 0) {
    loge_errno("worker socket accept failure");
    goto __err;
  }
  if (fd_addfl(pfd->fd, O_NONBLOCK) < 0) {
    loge_errno("worker socket unblock failure");
    goto __err;
  }
  char      *addr;
  in_port_t  port;
  if (getpeernamestr(pfd->fd, &addr, &port) < 0) {
    loge_errno("worker socket getpeername failure");
    goto __err;
  }

  worker_t *worker = worker_new(pfd, addr, ntohs(port));
  list_insert_tail(&worker->node, &server->workers);
  logi_worker(worker, "accepted");

  if (pollfd_pool_capped(server->pfdpool) && server->pfd->fd > 0) {
    logi("server pollfd DISABLED...");
    server->pfd->fd = -server->pfd->fd;
  }
  return SUCCESS;

__err:
  pollfd_pool_return(server->pfdpool, pfd);
  return FAILURE;
}

static int
server_worker_xchg(server_t *server) {
  int rc = IGNORED;
  worker_t *worker, *temp;
  worker_for_each_safe(worker, temp, &server->workers) {
    switch (rc = worker_xchg(worker)) {
      case PARTIAL: __fallthrough;
      case SUCCESS: __fallthrough;
      case IGNORED: break;
      default: {
        switch (rc) {
          case RUPTURE: logi_worker(worker, "rupture"); break;
          case FAILURE: loge_worker(worker, "failure"); break;
          default:      loge_worker(worker, "unknown"); break;
        }
        list_unlink(&worker->node);
        pollfd_pool_return(server->pfdpool, worker->pfd);
        worker_free(worker);

        if (server->pfd->fd < 0) {
          logi("server pollfd ENABLED...");
          server->pfd->fd = -server->pfd->fd;
        }
      }
    }
  }
  return IGNORED;
}

server_t *
server_new(const server_args_t *args, pollfd_pool_t *pfdpool) {
  pollfd_t *pfd = pollfd_pool_borrow(pfdpool);
  if (!pfd) {
    loge("server failed to borrow pollfd");
    return null;
  }
  if ((pfd->fd = server_socket(args->address, args->port, args->backlog)) < 0) {
    loge("server failed to create socket");
    pollfd_pool_return(pfdpool, pfd);
    return null;
  }
  server_t *server = malloc(sizeof(*server));
  server->pfd = pfd;
  server->pfd->events = POLLIN;
  server->pfd->revents = 0;
  server->pfdpool = pfdpool;
  server->workers = list_root(&server->workers);
  return server;
}

int
server_xchg(server_t *server) {
  if (server->pfd->revents & POLLERR) {
    loge("server pollfd POLLERR");
    return FAILURE;
  }
  if (server->pfd->revents & POLLNVAL) {
    loge("server pollfd POLLNVAL");
    return FAILURE;
  }
  if (server->pfd->revents & POLLHUP) {
    loge("server pollfd POLLHUP");
    return FAILURE;
  }

  int rc = IGNORED;
  switch ((rc = server_worker_init(server))) {
    case IGNORED: break;
    default:      return rc;
  }
  switch ((rc = server_worker_xchg(server))) {
    case IGNORED: break;
    default:      return rc;
  }
  return IGNORED;
}

void
server_free(server_t *server) {
  if (!server) return;
  worker_t *worker, *temp;
  worker_for_each_safe(worker, temp, &server->workers) {
    list_unlink(&worker->node);
    worker_free(worker);
  }
  free(server);
}