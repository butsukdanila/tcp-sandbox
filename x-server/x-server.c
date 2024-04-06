#define _POSIX_C_SOURCE 200112L // getaddrinfo etc.

#include "x-server-api.h"
#include "x-logs.h"
#include "x-misc.h"
#include "x-inet.h"

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#define ADDR_DEF "0.0.0.0"
#define PORT_DEF "4200"
#define BLOG_DEF 20

typedef struct {
  char * addr;
  char * port;
  int    blog;
} xs_args_t;

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
    "    -b | --backlog   listen backlog (defaults to %d)\n"
    "    -h | --help      display this text\n"
    "\n",
    program, ADDR_DEF, PORT_DEF, BLOG_DEF
  );
}

static void __parse_args(xs_args_t * args, int argc, char ** argv) {
  const char          __opts_s[] = "a:p:b:h";
  const struct option __opts_l[] = {
    { "address", required_argument, NULL, 'a' },
    { "port",    required_argument, NULL, 'p' },
    { "backlog", required_argument, NULL, 'b' },
    { "help",    no_argument,       NULL, 'h' },
    { /*sentinel*/ }
  };

  int i, c;
  while ((c = getopt_long(argc, argv, __opts_s, __opts_l, &i)) != -1) {
    switch(c) {
      case 'a': args->addr = optarg;       break;
      case 'p': args->port = optarg;       break;
      case 'b': args->blog = atoi(optarg); break;
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

static void __xs_op_ping(const xs_frame_t * req __attribute__((unused)), xs_frame_t * rsp) {
  logi("process...");
  static const char pong[] = "pong";
  xs_frame_body_set(rsp, pong, sizeof(pong));
  logi("success");
}

int main(int argc, char ** argv) {
  int rc = 0;
  xs_args_t * args = &(xs_args_t) {
    .addr = ADDR_DEF,
    .port = PORT_DEF,
    .blog = BLOG_DEF
  };
  __parse_args(args, argc, argv);
  logi("selected address: %s", args->addr);
  logi("selected port:    %s", args->port);
  logi("selected backlog: %d", args->blog);

  ai_t s_ai_hint = {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  ai_t * s_ai = NULL;
  rc = getaddrinfo(args->addr, args->port, &s_ai_hint, &s_ai);
  if (rc < 0) {
    loge("getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    goto_rc(rc, EXIT_FAILURE, __return_rc);
  }
  u16    s_port     = sa_port(s_ai->ai_addr);
  char * s_addr_str = sa_addr_str(s_ai->ai_addr);
  if (!s_addr_str) {
    loge_errno("inet_ntop failure");
    goto_rc(rc, EXIT_FAILURE, __return_rc);
  }
  logi("using: %s:%d", s_addr_str, s_port);
  free(s_addr_str);

  int s_sock = socket(s_ai->ai_family, s_ai->ai_socktype, s_ai->ai_protocol);
  if (s_sock < 0) {
    loge_errno("socket failure");
    goto_rc(rc, EXIT_FAILURE, __freeaddrinfo_s_sa_info);
  }
  logi("socket success...");

  int optval = 1;
  rc = setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (rc < 0) {
    loge_errno("setsockopt failure");
    goto_rc(rc, EXIT_FAILURE, __close_s_sock);
  }
  logi("setsockopt success...");

  rc = bind(s_sock, s_ai->ai_addr, s_ai->ai_addrlen);
  if (rc < 0) {
    loge_errno("bind failure");
    goto_rc(rc, EXIT_FAILURE, __close_s_sock);
  }
  logi("bind success...");

  rc = listen(s_sock, args->blog);
  if (rc < 0) {
    loge_errno("listen failure");
    goto_rc(rc, EXIT_FAILURE, __close_s_sock);
  }
  logi("listen success...");

  sa_storage_t c_sa_storage    = {};
  sl_t         c_sa_storage_sz = sizeof(c_sa_storage);
  while (1) {
    logi("waiting for connection...");
    int c_sock = accept(s_sock, (sa_t *)(&c_sa_storage), &c_sa_storage_sz);
    if (c_sock < 0) {
      loge_errno("accept failure");
      break;
    }
    u16    c_port     = sa_port(&c_sa_storage);
    char * c_addr_str = sa_addr_str(&c_sa_storage);
    logi("established connection: %s:%d", c_addr_str, c_port);
    free(c_addr_str);

    xs_frame_t req = {};
    xs_frame_t rsp = {};
    while (1) {
      xs_frame_zero(&req);
      xs_frame_zero(&rsp);

      rsp.head.op_type = XS_OP_TYPE_RSP;
      if (xs_frame_recv(c_sock, &req) < 0) {
        loge_errno("xs_frame_recv failure");
        break;
      }

#define __xs_op_case(_code, _func) case _code: _func(&req, &rsp); break
      switch ((short)req.head.op_code) {
        __xs_op_case(XS_OP_PING, __xs_op_ping);
        default: {
          xs_error_any(&rsp, XS_ERROR_NOP, "unknown operation: %u", req.head.op_code);
          break;
        }
      }
#undef __xs_op_case

      if (xs_frame_send(c_sock, &rsp) < 0) {
        loge_errno("xs_frame_recv failure");
        break;
      }
    }
    xs_frame_dispose(&req);
    xs_frame_dispose(&rsp);
    close(c_sock);
  }

__close_s_sock:
  close(s_sock);
__freeaddrinfo_s_sa_info:
  freeaddrinfo(s_ai);
__return_rc:
  return rc;
}