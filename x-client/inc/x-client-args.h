#ifndef __X_CLIENT_ARGS_H__
#define __X_CLIENT_ARGS_H__

#include "x-types.h"
#include <sys/time.h>

#define ADDRESS_DEF  "127.0.0.1"
#define PORT_DEF     "4200"
#define COUNT_DEF    -1
#define DELAY_DEF    0
#define INTERVAL_DEF 0

typedef struct {
  char   *address;
  char   *port;
  char   *message;
  i32     count;
  time_t  delay;
  time_t  interval;
} client_args_t;

#define client_args_default() \
  (client_args_t) {           \
    .address  = ADDRESS_DEF,  \
    .port     = PORT_DEF,     \
    .count    = COUNT_DEF,    \
    .delay    = DELAY_DEF,    \
    .interval = INTERVAL_DEF  \
  }

void
client_args_parse(int argc, char **argv, client_args_t *args);

#endif//__X_CLIENT_ARGS_H__