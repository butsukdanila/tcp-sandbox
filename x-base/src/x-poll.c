#include "x-poll.h"
#include "x-misc.h"

#include <sys/fcntl.h>
#include <sys/socket.h>

int
x_pollfd_send(x_pollfd_ctx_t * ctx) {
  bool    nbk = x_fd_has_flag(ctx->pfd->fd, O_NONBLOCK);
  u08 *   buf = ctx->buf;
  ssize_t snt = 0;
  do {
    snt = send(ctx->pfd->fd, buf, ctx->trg_sz + ctx->cur_sz, 0);
    if (snt < 0) {
      return -1;
    }
    if (snt == 0) {
      return -2;
    }
    buf         += (size_t)snt;
    ctx->cur_sz += (size_t)snt;
  } while (!nbk && ctx->cur_sz < ctx->trg_sz);

  return ctx->cur_sz < ctx->trg_sz ? 1 : 0;
}

int
x_pollfd_recv(x_pollfd_ctx_t * ctx) {
  bool    nbk = x_fd_has_flag(ctx->pfd->fd, O_NONBLOCK);
  u08 *   buf = ctx->buf;
  ssize_t rcv = 0;
  do {
    rcv = recv(ctx->pfd->fd, buf, ctx->trg_sz - ctx->cur_sz, 0);
    if (rcv < 0) {
      return -1;
    }
    if (rcv == 0) {
      return -2;
    }
    buf         += (size_t)rcv;
    ctx->cur_sz += (size_t)rcv;
  } while (!nbk && ctx->cur_sz < ctx->trg_sz);

  return ctx->cur_sz < ctx->trg_sz ? 1 : 0;
}