#ifndef __X_CLIENT_H__
#define __X_CLIENT_H__

#include "x-client-args.h"
#include "x-poll-pool.h"

struct client;
typedef struct client client_t;

client_t *
client_new(client_args_t *args, pollfd_pool_t *pfdpool);

int
client_xchg(client_t *client);

void
client_free(client_t *client);

#endif//__X_CLIENT_H__
