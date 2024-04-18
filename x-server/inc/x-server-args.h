#ifndef __X_SERVER_ARGS_H__
#define __X_SERVER_ARGS_H__

#include "x-types.h"

#define ADDRESS_DEF     "0.0.0.0"
#define PORT_DEF        "4200"
#define BACKLOG_DEF     20
#define CLIENTS_CAP_DEF 10

typedef struct {
  char   *address;
  char   *port;
  int     backlog;
  size_t  clients_cap;
} server_args_t;

#define server_args_default()       \
  (server_args_t) {                 \
    .address     = ADDRESS_DEF,     \
    .port        = PORT_DEF,        \
    .backlog     = BACKLOG_DEF,     \
    .clients_cap = CLIENTS_CAP_DEF, \
  }

void
server_args_parse(int argc, char **argv, server_args_t *args);

#endif//__X_SERVER_ARGS_H__