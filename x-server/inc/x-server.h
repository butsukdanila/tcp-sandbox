#ifndef __X_SERVER_H__
#define __X_SERVER_H__

#include "x-server-args.h"
#include "x-poll-pool.h"

struct server;
typedef struct server server_t;

server_t *
server_new(const server_args_t *args, pollfd_pool_t *pfdpool);

int
server_xchg(server_t *server);

void
server_free(server_t *server);

#endif//__X_SERVER_H__