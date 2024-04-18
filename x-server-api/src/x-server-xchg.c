#include "x-server-xchg.h"
#include "x-logs.h"

#include <stdlib.h>
#include <unistd.h>

void
server_xchg_recv_prep(server_xchg_t *xchg) {
  server_frame_zero(&xchg->frame);
  xchg->state = SXG_ST_HEAD;
  xchg->pfdxg.pfd->events = POLLIN;
  xchg->pfdxg.buf = &xchg->frame.head;
  xchg->pfdxg.trg_sz = sizeof(xchg->frame.head);
  xchg->pfdxg.cur_sz = 0;
}

void
server_xchg_send_prep(server_xchg_t *xchg) {
  xchg->state = SXG_ST_HEAD;
  xchg->pfdxg.pfd->events = POLLOUT;
  xchg->pfdxg.buf = &xchg->frame.head;
  xchg->pfdxg.trg_sz = sizeof(xchg->frame.head);
  xchg->pfdxg.cur_sz = 0;
}

void
server_xchg_disp(server_xchg_t *xchg) {
  if (!xchg) return;
  server_frame_disp(&xchg->frame);
}

int
server_xchg_recv(server_xchg_t *xchg) {
  if (!(xchg->pfdxg.pfd->revents & POLLIN)) {
    return IGNORED;
  }
  int rc = pollfd_xchg_recv(&xchg->pfdxg);
  switch (rc) {
    case SUCCESS: break;
    default:      return rc;
  }
  switch (xchg->state) {
    // request head received
    case SXG_ST_HEAD: {
      if (xchg->frame.head.body_sz) {
        xchg->state = SXG_ST_BODY;
        xchg->pfdxg.buf = server_frame_body_realloc(&xchg->frame, xchg->frame.head.body_sz);
        xchg->pfdxg.trg_sz = xchg->frame.head.body_sz;
        xchg->pfdxg.cur_sz = 0;
        // body recv required
        return PARTIAL;
      }
      // body ignored
      __fallthrough;
    }
    // request body received
    case SXG_ST_BODY: {
      return SUCCESS;
    }
  }
  loge("[frame xchg recv] something went horribly wrong");
  return FAILURE;
}

int
server_xchg_send(server_xchg_t *xchg) {
  if (!(xchg->pfdxg.pfd->revents & POLLOUT)) {
    return IGNORED;
  }
  int rc = pollfd_xchg_send(&xchg->pfdxg);
  switch (rc) {
    case SUCCESS: break;
    default:      return rc;
  }
  switch (xchg->state) {
    // response head sent
    case SXG_ST_HEAD: {
      if (xchg->frame.head.body_sz) {
        xchg->state = SXG_ST_BODY;
        xchg->pfdxg.buf = xchg->frame.body;
        xchg->pfdxg.trg_sz = xchg->frame.head.body_sz;
        xchg->pfdxg.cur_sz = 0;
        // body send required
        return PARTIAL;
      }
      // body ignored
      __fallthrough;
    }
    // response body sent
    case SXG_ST_BODY: {
      return SUCCESS;
    }
  }
  loge("[frame xchg send] something went horribly wrong");
  return FAILURE;
}