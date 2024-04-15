#include "x-poll.h"
#include "x-misc.h"
#include "x-array.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include <sys/fcntl.h>
#include <sys/socket.h>

struct pollfd_pool {
	array_t *arr;
};

static void
pollfd_disp(void * obj) {
	pollfd_t *pfd = obj;
	if (!pfd) return;
	if (pfd->fd == INT_MIN) return;
	close(pfd->fd);
}

pollfd_pool_t *
pollfd_pool_new(size_t capacity) {
	pollfd_pool_t *ret = malloc(sizeof(*ret));
	ret->arr = array_new((arraycfg_t) {
		.cap = capacity,
		.obj_size = sizeof(pollfd_t),
		.obj_disp = pollfd_disp
	});
	return ret;
}

void
pollfd_pool_free(pollfd_pool_t *pool) {
	if (!pool) return;
	array_free(pool->arr);
	free(pool);
}

int
pollfd_pool_poll(pollfd_pool_t *pool, int timeout) {
	return poll(pool->arr->buf, pool->arr->len, timeout);
}

pollfd_t *
pollfd_pool_borrow(pollfd_pool_t *pool) {
	if (!array_capped(pool->arr)) {
		return array_req(pool->arr);
	}
	ssize_t i = (ssize_t)pool->arr->len;
	while (i--) {
		pollfd_t *pfd = array_get(pool->arr, (size_t)i);
		if (pfd->fd == INT_MIN) {
			return pfd;
		}
	}
	return null;
}

int
pollfd_pool_return(pollfd_pool_t *pool, pollfd_t * pfd) {
	ssize_t i = (ssize_t)pool->arr->len;
	while (i--) {
		if (pfd == array_get(pool->arr, (size_t)i)) {
			pollfd_disp(pfd);
			pfd->fd = INT_MIN;
			pfd->events = 0;
			pfd->revents = 0;
			return 0;
		}
	}
	return -1;
}

bool
pollfd_pool_capped(pollfd_pool_t *pool) {
	if (!array_capped(pool->arr)) {
		return false;
	}
	ssize_t i = (ssize_t)pool->arr->len;
	while (i--) {
		pollfd_t *pfd = array_get(pool->arr, (size_t)i);
		if (pfd->fd == INT_MIN) {
			return false;
		}
	}
	return true;
}