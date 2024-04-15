#include "x-server-api-xchg.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-poll.h"

#include <stdlib.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/socket.h>

int
server_xchg_ctx_send(server_xchg_ctx_t *ctx) {
  bool    nbk = fd_hasfl(ctx->pfd->fd, O_NONBLOCK);
  u08    *buf = ctx->buf;
  ssize_t snt = 0;
  do {
    snt = send(ctx->pfd->fd, buf, ctx->trg_sz + ctx->cur_sz, 0);
    if (snt < 0) {
      loge_errno("[xchg_ctx] send failure");
      return FAILURE;
    }
    if (snt == 0) {
      return RUPTURE;
    }
    buf         += (size_t)snt;
    ctx->cur_sz += (size_t)snt;
  } while (!nbk && ctx->cur_sz < ctx->trg_sz);

  return ctx->cur_sz < ctx->trg_sz ?
    PARTIAL :
    SUCCESS;
}

int
server_xchg_ctx_recv(server_xchg_ctx_t *ctx) {
  bool    nbk = fd_hasfl(ctx->pfd->fd, O_NONBLOCK);
  u08    *buf = ctx->buf;
  ssize_t rcv = 0;
  do {
    rcv = recv(ctx->pfd->fd, buf, ctx->trg_sz - ctx->cur_sz, 0);
    if (rcv < 0) {
      loge_errno("[xchg_ctx] recv failure");
      return FAILURE;
    }
    if (rcv == 0) {
      return RUPTURE;
    }
    buf         += (size_t)rcv;
    ctx->cur_sz += (size_t)rcv;
  } while (!nbk && ctx->cur_sz < ctx->trg_sz);

  return ctx->cur_sz < ctx->trg_sz ?
    PARTIAL :
    SUCCESS;
}

void
server_frame_xchg_recv_prep(server_frame_xchg_t *xchg) {
  server_frame_zero(&xchg->frame);
  xchg->state = SXS_HEAD;
  xchg->ctx.pfd->events = POLLIN;
  xchg->ctx.buf = &xchg->frame.head;
  xchg->ctx.trg_sz = sizeof(xchg->frame.head);
  xchg->ctx.cur_sz = 0;
}

void
server_frame_xchg_send_prep(server_frame_xchg_t *xchg) {
  xchg->state = SXS_HEAD;
  xchg->ctx.pfd->events = POLLOUT;
  xchg->ctx.buf = &xchg->frame.head;
  xchg->ctx.trg_sz = sizeof(xchg->frame.head);
  xchg->ctx.cur_sz = 0;
}

void
server_frame_xchg_disp(server_frame_xchg_t *xchg) {
  if (!xchg) return;
  server_frame_disp(&xchg->frame);
}

int
server_frame_xchg_recv(server_frame_xchg_t *xchg) {
  if (!(xchg->ctx.pfd->revents & POLLIN)) {
    return IGNORED;
  }
  int rc = server_xchg_ctx_recv(&xchg->ctx);
  switch (rc) {
    case SUCCESS: break;
    default:          return rc;
  }
  switch (xchg->state) {
    // request head received
    case SXS_HEAD: {
      if (xchg->frame.head.body_sz) {
        server_frame_body_realloc(&xchg->frame, xchg->frame.head.body_sz);
        xchg->state = SXS_BODY;
        xchg->ctx.buf = xchg->frame.body;
        xchg->ctx.trg_sz = xchg->frame.head.body_sz;
        xchg->ctx.cur_sz = 0;
        // body recv required
        return PARTIAL;
      }
      // body ignored
      __fallthrough;
    }
    // request body received
    case SXS_BODY: {
      return SUCCESS;
    }
  }
  loge("[frame xchg recv] something went horribly wrong");
  return FAILURE;
}

int
server_frame_xchg_send(server_frame_xchg_t *xchg) {
  if (!(xchg->ctx.pfd->revents & POLLOUT)) {
    return IGNORED;
  }
  int rc = server_xchg_ctx_send(&xchg->ctx);
  switch (rc) {
    case SUCCESS: break;
    default:          return rc;
  }
  switch (xchg->state) {
    // response head sent
    case SXS_HEAD: {
      if (xchg->frame.head.body_sz) {
        xchg->state = SXS_BODY;
        xchg->ctx.buf = xchg->frame.body;
        xchg->ctx.trg_sz = xchg->frame.head.body_sz;
        xchg->ctx.cur_sz = 0;
        // body send required
        return PARTIAL;
      }
      // body ignored
      __fallthrough;
    }
    // response body sent
    case SXS_BODY: {
      return SUCCESS;
    }
  }
  loge("[frame xchg send] something went horribly wrong");
  return FAILURE;
}