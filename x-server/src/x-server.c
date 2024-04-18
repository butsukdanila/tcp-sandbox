#include "x-server.h"
#include "x-server-api.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-inet.h"
#include "x-list.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

typedef enum {
  CXS_RECV = 0,
  CXS_SEND = 1,
} client_xchg_state_t;

typedef struct {
  client_xchg_state_t  state;
  server_xchg_t        xchg;
  char                *addr;
  in_port_t            port;
  size_t               msgc;
  list_t               node;
} client_t;

#define client_for_each_safe(_data, _temp, _head) \
  list_for_each_data_safe(_data, _temp, _head, node)

#define logi_client(_a, _f, ...) \
  logi("client[%d] " _f, (_a)->port, ##__VA_ARGS__)

#define loge_client(_a, _f, ...) \
  loge("client[%d] " _f, (_a)->port, ##__VA_ARGS__)

static client_t *
client_init(pollfd_t *pfd, char *addr, in_port_t port) {
  client_t *client = malloc(sizeof(*client));
  client->state = CXS_RECV;
  client->xchg = (server_xchg_t) { .pfdxg.pfd = pfd };
  server_xchg_recv_prep(&client->xchg);
  client->addr = addr;
  client->port = port;
  client->msgc = 0;
  return client;
}

static void
client_disp(void *obj) {
  client_t *client = obj;
  if (!client) return;
  free(client->addr);
  server_xchg_disp(&client->xchg);
}

static void
client_free(void *obj) {
  client_t *client = obj;
  if (!client) return;
  client_disp(client);
  free(client);
}

static void
server_op_ping(client_t *client __unused, server_frame_t *rsp) {
  static const char pong[] = "pong";
  server_frame_body_set(rsp, pong, sizeof(pong));
}

static void
server_op_message(client_t *client, server_frame_t *rsp __unused) {
  logi_client(client, "MESSAGE[%03zu]: %s",
    client->msgc++,
    (char *)client->xchg.frame.body);
}

static server_frame_t
server_op_callback(client_t *client) {
  const server_frame_t *req = &client->xchg.frame;
  server_frame_t       *rsp = &server_frame_rsp();
  switch (req->head.req.opcode) {
    case SREQ_OP_PING:    server_op_ping(client, rsp);    break;
    case SREQ_OP_MESSAGE: server_op_message(client, rsp); break;
    default: {
      server_error_any(rsp, SERR_NOP, "unknown: %u", req->head.req.opcode);
      break;
    }
  }
  return *rsp;
}

static int
client_xchg(client_t *client) {
  if (client->state == CXS_RECV) {
    int rc = server_xchg_recv(&client->xchg);
    if (rc == SUCCESS) {
      server_frame_t rsp = server_op_callback(client);
      server_xchg_disp(&client->xchg);
      client->xchg.frame = rsp;
      server_xchg_send_prep(&client->xchg);
      client->state = CXS_SEND;
    }
    // logi_client(client, "[RECV] %d", rc);
    return rc;
  }
  if (client->state == CXS_SEND) {
    int rc = server_xchg_send(&client->xchg);
    if (rc == SUCCESS) {
      server_xchg_recv_prep(&client->xchg);
      client->state = CXS_RECV;
    }
    // logi_client(client, "[SEND] %d", rc);
    return rc;
  }
  loge_client(client, "something went horribly wrong");
  return FAILURE;
}

struct server {
  pollfd_t      *pfd;
  pollfd_pool_t *pfdpool;
  list_t         clients;
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
server_client_init(server_t *server) {
  if (!(server->pfd->revents & POLLIN)) {
    return IGNORED;
  }
  pollfd_t *pfd = pollfd_pool_borrow(server->pfdpool);
  if (!pfd) {
    loge("client failed to borrow pollfd");
    return FAILURE;
  }
  if ((pfd->fd = accept(server->pfd->fd, null, null)) < 0) {
    loge_errno("client socket accept failure");
    goto __err;
  }
  if (fd_addfl(pfd->fd, O_NONBLOCK) < 0) {
    loge_errno("client socket unblock failure");
    goto __err;
  }
  char      *addr;
  in_port_t  port;
  if (getpeernamestr(pfd->fd, &addr, &port) < 0) {
    loge_errno("client socket getpeername failure");
    goto __err;
  }

  client_t *client = client_init(pfd, addr, ntohs(port));
  list_insert_tail(&client->node, &server->clients);
  logi_client(client, "accepted");

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
server_client_xchg(server_t *server) {
  int rc = IGNORED;
  client_t *client, *temp;
  client_for_each_safe(client, temp, &server->clients) {
    switch (rc = client_xchg(client)) {
      case PARTIAL: __fallthrough;
      case SUCCESS: __fallthrough;
      case IGNORED: break;
      default: {
        switch (rc) {
          case RUPTURE: logi_client(client, "rupture"); break;
          case FAILURE: loge_client(client, "failure"); break;
          default:      loge_client(client, "unknown"); break;
        }
        pollfd_pool_return(server->pfdpool, client->xchg.pfdxg.pfd);
        list_unlink(&client->node);
        client_free(client);

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
server_init(const server_args_t *args, pollfd_pool_t *pfdpool) {
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
  memset(server, 0, sizeof(*server));
  server->pfd = pfd;
  server->pfd->events = POLLIN;
  server->pfd->revents = 0;
  server->pfdpool = pfdpool;
  server->clients = list_root(&server->clients);
  return server;
}

int
server_call(server_t *server) {
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

  int rc = 0;
  switch ((rc = server_client_init(server))) {
    case IGNORED: break;
    default:      return rc;
  }
  switch ((rc = server_client_xchg(server))) {
    case IGNORED: break;
    default:      return rc;
  }
  return IGNORED;
}

void
server_free(server_t *server) {
  if (!server) return;
  client_t *client, *temp;
  client_for_each_safe(client, temp, &server->clients) {
    list_unlink(&client->node);
    client_free(client);
  }
  free(server);
}