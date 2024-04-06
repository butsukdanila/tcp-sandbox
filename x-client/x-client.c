#define _GNU_SOURCE // getline()
#include "x-server-api.h"
#include "x-logs.h"
#include "x-inet.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define ADDR_DEF "127.0.0.1"
#define PORT_DEF "4200"

#define xs_frame_xchg_ex(_fd, _req, _rsp) ({     \
  int rc = xs_frame_xchg((_fd), (_req), (_rsp)); \
  if (rc == -1) {                                \
    loge_errno("xs_frame_send failure");         \
  } else if (rc == -2) {                         \
    loge_errno("xs_frame_recv failure");         \
  } else if (rc == -3) {                         \
    xs_error_t * err = (_rsp)->body;             \
    loge(                                        \
      "[XERR][%u][%u]: %s",                      \
      err->auth, err->code, err->text            \
    );                                           \
  }                                              \
  rc;                                            \
})

typedef struct {
  char * addr;
  char * port;
} xc_args_t;

static void __usage(FILE * file, char * program) {
  fprintf(file,
    "Usage: %s [opts] [addr [port]]\n"
    "\n"
    "Args:\n"
    "    addr             address to bind socket to (defaults to %s)\n"
    "    port             port to bind to (defaults to %s)\n"
    "\n"
    "Opts:\n"
    "    -a | --address   same as 'addr' argument\n"
    "    -p | --port      same as 'port' argument\n"
    "    -h | --help      display this text\n"
    "\n",
    program, ADDR_DEF, PORT_DEF
  );
}

static void __parse_args(xc_args_t * args, int argc, char ** argv) {
  const char          __opts_s[] = "a:p:h";
  const struct option __opts_l[] = {
    { "address", required_argument, NULL, 'a' },
    { "port",    required_argument, NULL, 'p' },
    { "help",    no_argument,       NULL, 'h' },
    { /*sentinel*/ }
  };

  int i, c;
  while ((c = getopt_long(argc, argv, __opts_s, __opts_l, &i)) != -1) {
    switch(c) {
      case 'a': args->addr = optarg;       break;
      case 'p': args->port = optarg;       break;
      case 'h': __usage(stdout, argv[0]);  exit(EXIT_SUCCESS);
      default : __usage(stderr, argv[0]);  exit(EXIT_FAILURE);
    }
  }
  if (optind < argc) {
    args->addr = argv[optind++];
    if (optind < argc) {
      args->port = argv[optind++];
    }
  }
}

typedef enum {
  XC_CMD_PING = 0
} xc_cmd_t;

static int xc_cmd_ping(int sock) {
  logi("ping...");
  xs_frame_t req = xs_frame_req(XS_OP_PING);
  xs_frame_t rsp = {};
  if (xs_frame_xchg_ex(sock, &req, &rsp) < 0) {
    return -1;
  }

  logi("...%s", (char *)rsp.body);
  xs_frame_dispose(&req);
  xs_frame_dispose(&rsp);
  return 0;
}

int main(int argc, char ** argv) {
  xc_args_t * args = &(xc_args_t) {
    .addr = ADDR_DEF,
    .port = PORT_DEF,
  };
  __parse_args(args, argc, argv);

  struct sockaddr_in s_addr = {
    .sin_family      = AF_INET,
    .sin_addr.s_addr = args->addr ? inet_addr(args->addr) : htonl(INADDR_LOOPBACK),
    .sin_port        = htons((in_port_t)(args->port ? atoi(args->port) : 4200))
  };
  logi("target server: %s:%d", inet_ntoa(s_addr.sin_addr), ntohs(s_addr.sin_port));

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    loge_errno("socket failure");
    return EXIT_FAILURE;
  }
  logi("socket success...");

  if (connect(sock, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
    loge_errno("connect failure");
    return EXIT_FAILURE;
  }
  logi("connect success...");
  fprintf(stdout,
    "commands:\n"
    "  XC_CMD_PING = 0\n"
  );
  char * line = NULL;
  size_t linelen = 0;
  while (1) {
    fprintf(stdout, "enter command number: ");
    if (getline(&line, &linelen, stdin) < 0) {
      break;
    }
    xc_cmd_t cmd = (xc_cmd_t)atoi(line);
#define xc_cmd_case(_c, _f) case _c: _f(sock); break
    switch (cmd) {
      xc_cmd_case(XC_CMD_PING, xc_cmd_ping);
      default: {
        loge("unknown command: %d", cmd);
        break;
      }
    }
#undef  xc_cmd_case
  }

  free(line);
  close(sock);
  return EXIT_SUCCESS;
}