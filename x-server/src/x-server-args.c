#include "x-server-args.h"
#include "x-misc.h"

#include <stdlib.h>
#include <argp.h>

static
error_t
__parser(int key, char *arg, struct argp_state *state) {
  server_args_t *args = state->input;
  switch (key) {
    case 'a': args->address     = arg;               break;
    case 'p': args->port        = arg;               break;
    case 'b': args->backlog     = atoi(arg);         break;
    case 's': args->clients_cap = (size_t)atol(arg); break;
    case ARGP_KEY_ARG: {
      switch (state->arg_num) {
        case 0:  args->address = arg; break;
        case 1:  args->port    = arg; break;
        default: argp_usage(state);   break;
      }
      break;
    }
    default: {
      return ARGP_ERR_UNKNOWN;
    }
  }
  return 0;
}

void
server_args_parse(int argc, char **argv, server_args_t *args) {
  argp_parse(
    &(struct argp) {
      .args_doc = "[ADDR [PORT]]",
      .doc      = "Start TPC server with socket bound to ADDR:PORT",
      .parser   = __parser,
      .options  = (struct argp_option[]) {
        { "address",     'a', "ADDR", 0, "Address to bind socket to (default: "ADDRESS_DEF")",          0 },
        { "port",        'p', "PORT", 0, "Port to bind socket to (default: "PORT_DEF")",                0 },
        { "backlog",     'b', "BLOG", 0, "Listen backlog (default: "tostr(BACKLOG_DEF)")",              1 },
        { "clients-cap", 's', "CMAX", 0, "Max allowed connections (default: "tostr(CLIENTS_CAP_DEF)")", 2 },
        { /*sentinel*/ }
      }
    },
    argc, argv, 0, 0, args
  );
}