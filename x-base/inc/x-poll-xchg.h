#ifndef __X_POLL_XCHG_H__
#define __X_POLL_XCHG_H__

#include "x-poll.h"

struct pollfd_xchg {
  pollfd_t *pfd;
  void     *buf;
  size_t    trg_sz;
  size_t    cur_sz;
};
typedef struct pollfd_xchg pollfd_xchg_t;

int
pollfd_xchg_send(pollfd_xchg_t *xchg);

int
pollfd_xchg_recv(pollfd_xchg_t *xchg);

#endif//__X_POLL_XCHG_H__
