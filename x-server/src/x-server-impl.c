#define _POSIX_C_SOURCE 200112L // getaddrinfo etc.

#include "x-server-impl.h"
#include "x-server-api.h"
#include "x-logs.h"
#include "x-inet.h"
#include "x-misc.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fcntl.h>

typedef enum {
  XS_ACT_REQ_HEAD = 0,
  XS_ACT_REQ_BODY = 1,
  XS_ACT_RSP_HEAD = 2,
  XS_ACT_RSP_BODY = 3,
} xs_act_t;

struct xs_actor {
  xs_frame_t     frm;
  x_pollfd_ctx_t ctx;
  xs_act_t       act;
};

#define logi_actor(_a, _f, ...) \
  logi("actor[%d]: " _f, (_a)->ctx.pfd->fd, ##__VA_ARGS__)

#define loge_actor(_a, _f, ...) \
  loge("actor[%d]: " _f, (_a)->ctx.pfd->fd, ##__VA_ARGS__)

static
void
xs_op_ping(xs_frame_t const * req __unused, xs_frame_t * rsp) {
  logi("process...");
  static const char pong[] = "pong";
  xs_frame_body_set(rsp, pong, sizeof(pong));
  logi("success");
}

static
xs_frame_t
xs_handle(xs_actor_t const * actor) {
  xs_frame_t const * req = &actor->frm;
  xs_frame_t       * rsp = &xs_frame_rsp();

  logi_actor(actor, "%u", req->head.op_code);
  switch (req->head.op_code) {
    case XS_OP_PING: {
      xs_op_ping(req, rsp);
      break;
    }
    default: {
      xs_error_any(rsp, XS_ERROR_NOP, "unknown operation: %u", req->head.op_code);
      break;
    }
  }
  return *rsp;
}

int
xs_socket(const char * address, const char * port, const i32 backlog) {
  logi("retrieving address info...");
  addrinfo_t * ai_hint = &(addrinfo_t) {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  addrinfo_t * ai = NULL;
  int rc = getaddrinfo(address, port, ai_hint, &ai);
  if (rc < 0) {
    loge("getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    return -1;
  }

  logi("socket triage...");
  int fd = -1;
  int on =  1;
  for (addrinfo_t * p = ai; p != NULL; p = p->ai_next) {
    logi("socket create...");
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) {
      loge_errno("socket failure");
      continue;
    }
    logi("socket unblock...");
    if (x_fd_add_flag(fd, O_NONBLOCK) < 0) {
      loge_errno("socket unblock failure");
      goto __triage_next;
    }
    logi("socket setup...");
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
      loge_errno("setsockopt failure");
      goto __triage_next;
    }
    logi("socket bind...");
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
      loge_errno("bind failure");
      goto __triage_next;
    }
    logi("socket listen (%d)...", backlog);
    if (listen(fd, backlog) < 0) {
      loge_errno("listen failure");
      goto __triage_next;
    }
    logi("socket triage success");
    break;

  __triage_next:
    close(fd);
    fd = -1;
  }

  if (fd < 0) {
    loge("socket triage failure");
    goto __freeaddrinfo_ai;
  }

  char *    in_addr = sockaddr_in_addrstr(ai->ai_addr);
  in_port_t in_port = sockaddr_in_port(ai->ai_addr);
  logi("socket ready: %s:%d", in_addr, ntohs(in_port));
  free(in_addr);

__freeaddrinfo_ai:
  freeaddrinfo(ai);
  return fd;
}

size_t
xs_actor_size() {
  return sizeof(xs_actor_t);
}

xs_actor_t *
xs_actor_init(pollfd_t * pfd) {
  xs_actor_t * ret = malloc(sizeof(*ret));
  ret->frm             = (xs_frame_t) {};
  ret->act             = XS_ACT_REQ_HEAD;
  ret->ctx.pfd         = pfd;
  ret->ctx.pfd->events = POLLIN;
  ret->ctx.buf         = &ret->frm.head;
  ret->ctx.trg_sz      = sizeof(ret->frm.head);
  ret->ctx.cur_sz      = 0;
  return ret;
}

int
xs_actor_call(xs_actor_t * actor) {
  if (actor->ctx.pfd->revents & POLLHUP) {
    logi_actor(actor, "hung up");
    return -1; // hung up
  }

  if (actor->ctx.pfd->revents & POLLIN) {
    switch (x_pollfd_recv(&actor->ctx)) {
      case  1: return 0;      // recv partial
      case  0: break;         // recv success
      case -1: __fallthrough; // recv failure
      case -2: __fallthrough; // hung up
      default: return -1;     // unknown
    }

    switch (actor->act) {
      case XS_ACT_REQ_HEAD: {
        if (actor->frm.head.body_sz) {
          xs_frame_body_realloc(&actor->frm, actor->frm.head.body_sz);
          actor->act        = XS_ACT_REQ_BODY;
          actor->ctx.buf    = actor->frm.body;
          actor->ctx.trg_sz = actor->frm.head.body_sz;
          actor->ctx.cur_sz = 0;
          // body recv required (continuation)
          return 0;
        }
        __fallthrough; // body ignored
      }
      case XS_ACT_REQ_BODY: {
        actor->frm             = xs_handle(actor);
        actor->act             = XS_ACT_RSP_HEAD;
        actor->ctx.pfd->events = POLLOUT;
        actor->ctx.buf         = &actor->frm.head;
        actor->ctx.trg_sz      = sizeof(actor->frm.head);
        actor->ctx.cur_sz      = 0;
        break;
      }
      default: return -1; // something went horribly wrong
    }
  }

  if (actor->ctx.pfd->revents & POLLOUT) {
    switch (x_pollfd_send(&actor->ctx)) {
      case  1: return 0;      // send partial
      case  0: break;         // send success
      case -1: __fallthrough; // send failure
      case -2: __fallthrough; // hung up
      default: return -1;     // unknown code
    }

    // send success
    switch (actor->act) {
      case XS_ACT_RSP_HEAD: {
        if (actor->frm.head.body_sz) {
          actor->act        = XS_ACT_RSP_BODY;
          actor->ctx.buf    = actor->frm.body;
          actor->ctx.trg_sz = actor->frm.head.body_sz;
          actor->ctx.cur_sz = 0;
          // body send required (continuation)
          return 0;
        }
        __fallthrough; // body ignored
      }
      case XS_ACT_RSP_BODY: {
        xs_frame_zero(&actor->frm);
        actor->act             = XS_ACT_REQ_HEAD;
        actor->ctx.pfd->events = POLLIN;
        actor->ctx.buf         = &actor->frm.head;
        actor->ctx.trg_sz      = sizeof(actor->frm.head);
        actor->ctx.cur_sz      = 0;
        break;
      }
      default: return -1; // something went horribly wrong
    }
  }
  return 0;
}

void
xs_actor_disp(void * obj) {
  xs_actor_t * actor = obj;
  if (!actor) return;
  xs_frame_disp(&actor->frm);
}

void
xs_actor_free(void * obj) {
  xs_actor_disp(obj);
  free(obj);
}
