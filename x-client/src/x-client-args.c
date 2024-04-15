#include "x-client-args.h"
#include "x-misc.h"

#include <stdlib.h>
#include <argp.h>

static
error_t
__parser(int key, char *arg, struct argp_state *state) {
  client_args_t *args = state->input;
  switch (key) {
    case 'a': args->address = arg;       break;
    case 'p': args->port    = arg;       break;
    case 't': args->timeout = atol(arg); break;
    case 'm': args->message = arg;       break;
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
client_args_parse(int argc, char **argv, client_args_t *args) {
  argp_parse(
    &(struct argp) {
      .args_doc = "[ADDR [PORT]]",
      .doc      = "Start TCP client with socket bound to ADDR:PORT\n"
                  "that will send 'message' every 'timeout' seconds",
      .parser   = __parser,
      .options  = (struct argp_option[]) {
        { "address",      'a', "ADDR", 0, "Address to bind socket to (default: "XC_ADDRESS_DEF")",       0 },
        { "port",         'p', "PORT", 0, "Port to bind socket to (default: "XC_PORT_DEF")",             0 },
        { "timeout",      't', "Ns",   0, "Timeout between messages (default: "tostr(XC_TIMEOUT_DEF)")", 1 },
        { "message",      'm', "TEXT", 0, "Message text (default: \"x-client[pid]\")",                   1 },
        { /*sentinel*/ }
      }
    },
    argc, argv, 0, 0, args
  );
}