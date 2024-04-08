#ifndef __X_POLL_H__
#define __X_POLL_H__

#include "x-types.h"
#include <sys/poll.h>

typedef struct pollfd pollfd_t;

typedef struct {
  pollfd_t * pfd;
  void *     buf;
  size_t     trg_sz;
  size_t     cur_sz;
} x_pollfd_ctx_t;

int x_pollfd_send(x_pollfd_ctx_t * ctx);
int x_pollfd_recv(x_pollfd_ctx_t * ctx);

#endif//__X_POLL_H__