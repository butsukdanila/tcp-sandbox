#include "x-server.h"
#include "x-server-api.h"
#include "x-server-api-xchg.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-inet.h"
#include "x-list.h"

#include <assert.h>
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
  server_frame_xchg_t  xchg;
  char                *addr;
  in_port_t            port;
  size_t               msgc;
  list_t               node;
} client_t;

#define clients_foreach_safe(_data, _temp, _head) \
  list_foreach_data_safe(_data, _temp, _head, node)

#define logi_client(_a, _f, ...) \
  logi("client[%d] " _f, (_a)->port, ##__VA_ARGS__)

#define loge_client(_a, _f, ...) \
  loge("client[%d] " _f, (_a)->port, ##__VA_ARGS__)

static void
client_init(client_t *client, pollfd_t *pfd, char *addr, in_port_t port) {
  client->state = CXS_RECV;
  client->xchg.ctx.pfd = pfd;
  server_frame_xchg_recv_prep(&client->xchg);
  client->addr = addr;
  client->port = port;
  client->msgc = 0;
  logi_client(client, "created");
}

static void
client_disp(void *obj) {
  client_t *client = obj;
  if (!client) return;
  free(client->addr);
  server_frame_xchg_disp(&client->xchg);
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
  switch (req->head.op_code) {
    case SOPC_PING:    server_op_ping(client, rsp);    break;
    case SOPC_MESSAGE: server_op_message(client, rsp); break;
    default: {
      server_error_any(rsp, SE_NOP, "unknown: %u", req->head.op_code);
      break;
    }
  }
  return *rsp;
}

static int
client_xchg(client_t *client) {
  if (client->state == CXS_RECV) {
    int rc = server_frame_xchg_recv(&client->xchg);
    if (rc == SUCCESS) {
      client->xchg.frame = server_op_callback(client);
      server_frame_xchg_send_prep(&client->xchg);
      client->state = CXS_SEND;
    }
    // logi_client(client, "[RECV] %d", rc);
    return rc;
  }
  if (client->state == CXS_SEND) {
    int rc = server_frame_xchg_send(&client->xchg);
    if (rc == SUCCESS) {
      server_frame_xchg_recv_prep(&client->xchg);
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

  client_t *client = malloc(sizeof(*client));
  client_init(client, pfd, addr, ntohs(port));
  list_insert_tail(&client->node, &server->clients);

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
  clients_foreach_safe(client, temp, &server->clients) {
    switch (rc = client_xchg(client)) {
      case PARTIAL: __fallthrough;
      case SUCCESS: __fallthrough;
      case IGNORED: break;
      default: {
        switch (rc) {
          case RUPTURE: logi_client(client, "rupture"); break;
          case FAILURE: loge_client(client, "failure"); break;
          default:          loge_client(client, "unknown"); break;
        }
        pollfd_pool_return(server->pfdpool, client->xchg.ctx.pfd);
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
  assert(args);
  assert(pfdpool);

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
  server_t *ret = malloc(sizeof(*ret));
  ret->pfd = pfd;
  ret->pfd->events = POLLIN;
  ret->pfd->revents = 0;
  ret->pfdpool = pfdpool;
  ret->clients = list_root(&ret->clients);
  return ret;
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
  clients_foreach_safe(client, temp, &server->clients) {
    list_unlink(&client->node);
    client_free(client);
  }
  free(server);
}