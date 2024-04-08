#ifndef __X_SERVER_IMPL_H__
#define __X_SERVER_IMPL_H__

#include "x-poll.h"

struct xs_actor;
typedef struct xs_actor xs_actor_t;

int
xs_socket(const char * address, const char * port, const i32 backlog);

size_t
xs_actor_size();

xs_actor_t *
xs_actor_init(pollfd_t * pfd);

int
xs_actor_call(xs_actor_t * actor);

void
xs_actor_disp(void * obj);

void
xs_actor_free(void * obj);

#endif//__X_SERVER_IMPL_H__