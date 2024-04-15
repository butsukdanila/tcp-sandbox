#ifndef __X_CLIENT_ARGS_H__
#define __X_CLIENT_ARGS_H__

#include "x-types.h"
#include <time.h>

#define XC_ADDRESS_DEF "127.0.0.1"
#define XC_PORT_DEF    "4200"
#define XC_TIMEOUT_DEF 5

typedef struct {
  char   *address;
  char   *port;
  char   *message;
  time_t  timeout;
} client_args_t;

#define client_args_default()  \
  (client_args_t) {            \
    .address = XC_ADDRESS_DEF, \
    .port    = XC_PORT_DEF,    \
    .timeout = XC_TIMEOUT_DEF  \
  }

void
client_args_parse(int argc, char **argv, client_args_t *args);

#endif//__X_CLIENT_ARGS_H__