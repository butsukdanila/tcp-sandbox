#include "x-server-args.h"
#include "x-misc.h"

#include <stdlib.h>
#include <argp.h>

static error_t
__parser(int key, char * arg, struct argp_state * state) {
  xs_args_t * args = state->input;
  switch (key) {
    case 'a': args->address      = arg;       break;
    case 'p': args->port         = arg;       break;
    case 'b': args->backlog      = atoi(arg); break;
    case 'c': args->clients_size = atoi(arg); break;
    case 'C': args->clients_grow = true;      break;
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
xs_args_parse(int argc, char ** argv, xs_args_t * args) {
  argp_parse(
    &(struct argp) {
      .args_doc = "[ADDR [PORT]]",
      .doc      = "Start TPC server with socket bound to ADDR:PORT",
      .parser   = __parser,
      .options  = (struct argp_option[]) {
        { "address",      'a', "ADDR", 0, "Address to bind socket to (default: "XS_ADDRESS_DEF")",           0 },
        { "port",         'p', "PORT", 0, "Port to bind socket to (default: "XS_PORT_DEF")",                 0 },
        { "backlog",      'b', "BLOG", 0, "Listen backlog (default: "tostr(XS_BACKLOG_DEF)")",               1 },
        { "clients-size", 'c', "CMAX", 0, "Max allowed connections (default: "tostr(XS_CLIENTS_SIZE_DEF)")", 2 },
        { "clients-grow", 'C',      0, 0, "Allow growing over CMAX (default: "tostr(XS_CLIENTS_GROW_DEF)")", 2 },
        { /*sentinel*/ }
      }
    }, 
    argc, argv, 0, 0, args
  );  
}