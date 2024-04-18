#include "x-client-args.h"
#include "x-util.h"

#include <stdlib.h>
#include <argp.h>

static error_t
__parser(int key, char *arg, struct argp_state *state) {
  client_args_t *args = state->input;
  switch (key) {
    case 'a': args->address  = arg;       break;
    case 'p': args->port     = arg;       break;
    case 'm': args->message  = arg;       break;
    case 'n': args->count    = atoi(arg); break;
    case 'd': args->delay    = atol(arg); break;
    case 'i': args->interval = atol(arg); break;
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
                  "that will send 'message' every 'interval' seconds after 'delay' seconds 'count' times",
      .parser   = __parser,
      .options  = (struct argp_option[]) {
        { "address",  'a', "ADDR", 0, "Address to bind socket to (default: "ADDRESS_DEF")",         0 },
        { "port",     'p', "PORT", 0, "Port to bind socket to (default: "PORT_DEF")",               0 },
        { "message",  'm', "TEXT", 0, "Message text (default: \"client[pid]\")",                    1 },
        { "count",    'n', "N",    0, "Count of messages to send (default: "tostr(COUNT_DEF)")",    1 },
        { "delay",    'd', "Ns",   0, "Delay before first message (default: "tostr(DELAY_DEF)")",   2 },
        { "interval", 'i', "Ns",   0, "Interval between messages (default: "tostr(INTERVAL_DEF)")", 2 },
        { /*sentinel*/ }
      }
    },
    argc, argv, 0, 0, args
  );
}