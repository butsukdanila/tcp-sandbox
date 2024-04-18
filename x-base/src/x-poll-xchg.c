#include "x-poll-xchg.h"
#include "x-logs.h"
#include "x-util.h"

#include <sys/fcntl.h>
#include <sys/socket.h>

int
pollfd_xchg_send(pollfd_xchg_t *xchg) {
  bool    nbk = fd_hasfl(xchg->pfd->fd, O_NONBLOCK);
  u08    *buf = xchg->buf;
  ssize_t snt = 0;
  do {
    snt = send(xchg->pfd->fd, buf, xchg->trg_sz + xchg->cur_sz, 0);
    if (snt < 0) {
      loge_errno("[pollfd xchg] send failure");
      return FAILURE;
    }
    if (snt == 0) {
      return RUPTURE;
    }
    buf          += (size_t)snt;
    xchg->cur_sz += (size_t)snt;
  } while (!nbk && xchg->cur_sz < xchg->trg_sz);

  return xchg->cur_sz < xchg->trg_sz ?
    PARTIAL :
    SUCCESS;
}

int
pollfd_xchg_recv(pollfd_xchg_t *pfdx) {
  bool    nbk = fd_hasfl(pfdx->pfd->fd, O_NONBLOCK);
  u08    *buf = pfdx->buf;
  ssize_t rcv = 0;
  do {
    rcv = recv(pfdx->pfd->fd, buf, pfdx->trg_sz - pfdx->cur_sz, 0);
    if (rcv < 0) {
      loge_errno("[pollfd xchg] recv failure");
      return FAILURE;
    }
    if (rcv == 0) {
      return RUPTURE;
    }
    buf         += (size_t)rcv;
    pfdx->cur_sz += (size_t)rcv;
  } while (!nbk && pfdx->cur_sz < pfdx->trg_sz);

  return pfdx->cur_sz < pfdx->trg_sz ?
    PARTIAL :
    SUCCESS;
}