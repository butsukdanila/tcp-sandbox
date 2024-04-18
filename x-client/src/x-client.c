#include "x-client.h"
#include "x-server-api.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-inet.h"
#include "x-poll-timer.h"

#include <stdlib.h>
#include <unistd.h>

typedef enum {
  CST_WAIT = 0,
  CST_SEND = 1,
  CST_RECV = 2,
} client_state_t;

struct client {
  pollfd_t       *pfd;
  pollfd_pool_t  *pfdpool;
  char           *msg;
  size_t          msg_sz;
  i32             msgcnt;
  client_state_t  state;
  server_xchg_t   xchg;
  time_t          delay_sec;
  time_t          inter_sec;
  pollfd_t       *timer_pfd;
  u64             timer_exp;
};
typedef struct client client_t;

static int
client_socket(const char *address, const char *port) {
  addrinfo_t *ai_hint = &(addrinfo_t) {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  addrinfo_t *ai = null;
  int rc = getaddrinfo(address, port, ai_hint, &ai);
  if (rc < 0) {
    loge("client socket getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    return -1;
  }
  int fd = -1;
  for (addrinfo_t *p = ai; p; p = p->ai_next) {
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) {
      loge_errno("client socket failure");
      continue;
    }
    if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
      loge_errno("client socket connect failure");
      goto __probe_next;
    }
    if (fd_addfl(fd, O_NONBLOCK) < 0) {
      loge_errno("client socket unblock failure");
      goto __probe_next;
    }
    char      *in_addr = null;
    in_port_t  in_port = 0;
    if (getsocknamestr(fd, &in_addr, &in_port) < 0) {
      loge_errno("client socket getpeername failure");
      goto __probe_next;
    }
    logi("client ready: %s:%u", in_addr, ntohs(in_port));
    free(in_addr);
    break;

  __probe_next:
    close(fd);
    fd = -1;
  }
  freeaddrinfo(ai);
  return fd;
}

static void
client_xchg_wait_prep(client_t *client) {
  client->pfd->events = 0;
  client->pfd->revents = 0;
  client->state = CST_WAIT;
}

static void
client_xchg_send_prep(client_t *client) {
  server_frame_head_zero(&client->xchg.frame);
  client->xchg.frame.head.type = SFT_REQ;
  client->xchg.frame.head.req.opcode = SREQ_OP_MESSAGE;
  server_frame_body_set(&client->xchg.frame, client->msg, client->msg_sz);
  server_xchg_send_prep(&client->xchg);
  client->state = CST_SEND;
}

static void
client_xchg_recv_prep(client_t *client) {
  server_frame_head_zero(&client->xchg.frame);
  server_xchg_recv_prep(&client->xchg);
  client->state = CST_RECV;
}

client_t *
client_new(client_args_t *args, pollfd_pool_t *pfdpool) {
  pollfd_t *pfd = pollfd_pool_borrow(pfdpool);
  if (!pfd) {
    loge("client failed to borrow pollfd");
    return null;
  }
  if ((pfd->fd = client_socket(args->address, args->port)) < 0) {
    pollfd_pool_return(pfdpool, pfd);
    return null;
  }
  client_t *client = malloc(sizeof(*client));
  client->pfd = pfd;
  client->pfdpool = pfdpool;
  client->msg = args->message ?
    strdup(args->message) :
    strfmt("client[%d]", getpid());
  client->msg_sz = str_sz(client->msg);
  client->msgcnt = args->count ? args->count : (
    logi("message count can't be zero. defaulting to: "tostr(COUNT_DEF)"..."),
    (i32)(COUNT_DEF)
  );
  client->xchg = server_xchg_init(client->pfd);
  client->delay_sec = args->delay;
  client->inter_sec = args->interval;

  if (!client->delay_sec && !client->inter_sec) {
    client_xchg_send_prep(client);
    client->timer_pfd = null;
    return client;
  }

  pollfd_t *timer_pfd = pollfd_pool_borrow(pfdpool);
  if (!timer_pfd) {
    loge("client failed to borrow timer pollfd");
    return client;
  }
  if ((timer_pollfd_init(timer_pfd, client->delay_sec, client->inter_sec, TFD_NONBLOCK)) < 0) {
    pollfd_pool_return(pfdpool, timer_pfd);
    return client;
  }
  client_xchg_wait_prep(client);
  client->timer_pfd = timer_pfd;
  return client;
}

int
client_xchg(client_t *client) {
  if (client->state == CST_WAIT) {
    int rc = timer_pollfd_read(client->timer_pfd, &client->timer_exp);
    if (rc == SUCCESS) {
      if (client->delay_sec && !client->inter_sec) {
        pollfd_pool_return(client->pfdpool, client->timer_pfd);
        client->timer_pfd = null;
      }
      client_xchg_send_prep(client);
      return IGNORED;
    }
    return rc;
  }
  if (client->state == CST_SEND) {
    int rc = server_xchg_send(&client->xchg);
    if (rc == SUCCESS) {
      client_xchg_recv_prep(client);
      return IGNORED;
    }
    return rc;
  }
  if (client->state == CST_RECV) {
    int rc = server_xchg_recv(&client->xchg);
    if (rc == SUCCESS) {
      const server_error_t *err = server_error_get(&client->xchg.frame);
      if (err) {
        loge("server error [%u][%u]: %s", err->auth, err->code, err->text);
        return FAILURE;
      }

      if (client->msgcnt > 0) {
        client->msgcnt--;
      }
      if (client->timer_pfd) {
        if (client->msgcnt == 0) {
          pollfd_pool_return(client->pfdpool, client->timer_pfd);
          client->timer_pfd = null;
        } else {
          client_xchg_wait_prep(client);
        }
      } else {
        client_xchg_send_prep(client);
      }

      logi("sent (left: %d)", client->msgcnt);
      return client->msgcnt ? IGNORED : SUCCESS;
    }
  }
  return IGNORED;
}

void
client_free(client_t *client) {
  if (!client) return;
  free(client->msg);
  server_xchg_disp(&client->xchg);
  free(client);
}