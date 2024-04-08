#ifndef __X_SERVER_ARGS_H__
#define __X_SERVER_ARGS_H__

#include "x-types.h"

#define XS_ADDRESS_DEF      "0.0.0.0"
#define XS_PORT_DEF         "4200"
#define XS_BACKLOG_DEF      20
#define XS_CLIENTS_SIZE_DEF 1
#define XS_CLIENTS_GROW_DEF false

typedef struct {
  char * address;
  char * port;
  i32    backlog;
  i32    clients_size;
  bool   clients_grow;
} xs_args_t;

#define xs_args_default() \
  (xs_args_t) {                          \
    .address      = XS_ADDRESS_DEF,      \
    .port         = XS_PORT_DEF,         \
    .backlog      = XS_BACKLOG_DEF,      \
    .clients_size = XS_CLIENTS_SIZE_DEF, \
    .clients_grow = XS_CLIENTS_GROW_DEF  \
  }

void
xs_args_parse(int argc, char ** argv, xs_args_t * args);

#endif//__X_SERVER_ARGS_H__