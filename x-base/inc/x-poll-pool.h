#ifndef __X_POLL_POOL_H__
#define __X_POLL_POOL_H__

#include "x-poll.h"

struct pollfd_pool;
typedef struct pollfd_pool pollfd_pool_t;

pollfd_pool_t *
pollfd_pool_new(size_t capacity);

void
pollfd_pool_free(pollfd_pool_t *pool);

int
pollfd_pool_poll(pollfd_pool_t *pool, int timeout);

pollfd_t *
pollfd_pool_borrow(pollfd_pool_t *pool);

int
pollfd_pool_return(pollfd_pool_t *pool, pollfd_t * pfd);

bool
pollfd_pool_capped(pollfd_pool_t *pool);

#endif//__X_POLL_POOL_H__
