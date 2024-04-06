#define _POSIX_C_SOURCE 200112L // getaddrinfo etc.

#include "x-tcp.h"
#include "x-log.h"

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <alloca.h>

#include <netdb.h>
#include <arpa/inet.h>

#define ADDR_DEF "0.0.0.0"
#define PORT_DEF "4200"
#define BLOG_DEF 20

#define goto_rc(_rf, _rv, _l) ((_rf) = (_rv)); goto _l

#define sa_base(_sa) ((struct sockaddr     *)(_sa))
#define sa_ipv4(_sa) ((struct sockaddr_in  *)(_sa))
#define sa_ipv6(_sa) ((struct sockaddr_in6 *)(_sa))

typedef struct {
  char * addr;
  char * port;
  int    blog;
} args_t;

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

static void args_get(args_t * args, int argc, char ** argv) {
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

u16 inet_port_resolve(const void * __sa) {
  switch (sa_base(__sa)->sa_family) {
    case AF_INET:  return ntohs(sa_ipv4(__sa)->sin_port);
    case AF_INET6: return ntohs(sa_ipv6(__sa)->sin6_port);
  }
  return 0;
}

char * inet_addr_resolve(const void * __sa) {
  int af = sa_base(__sa)->sa_family;
  switch (af) {
    case AF_INET: {
      return (char *)inet_ntop(af, &sa_ipv4(__sa)->sin_addr,
        malloc(INET_ADDRSTRLEN), INET_ADDRSTRLEN
      );
    }
    case AF_INET6: {
      return (char *)inet_ntop(af, &sa_ipv6(__sa)->sin6_addr,
        malloc(INET6_ADDRSTRLEN), INET6_ADDRSTRLEN
      );
    };
    default: {
      loge("unknown address family: %d", af);
      return NULL;
    }
  }
}

static void __x_tcp_op_ping(const x_tcp_frame_t * req __attribute__((unused)), x_tcp_frame_t * rsp) {
  logi("process...");
  static const char pong[] = "pong";
  x_tcp_frame_body_set(rsp, pong, sizeof(pong));
  logi("success");
}

int main(int argc, char ** argv) {
  int rc = 0;
  args_t * args = &(args_t) {
    .addr = ADDR_DEF,
    .port = PORT_DEF,
    .blog = BLOG_DEF
  };
  args_get(args, argc, argv);
  logi("selected address: %s", args->addr);
  logi("selected port:    %s", args->port);
  logi("selected backlog: %d", args->blog);

  struct addrinfo * s_hint = &(struct addrinfo) {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  struct addrinfo * s_info = NULL;
  rc = getaddrinfo(args->addr, args->port, s_hint, &s_info);
  if (rc < 0) {
    loge("getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    goto_rc(rc, EXIT_FAILURE, __return_rc);
  }
  u16    s_port     = inet_port_resolve(s_info->ai_addr);
  char * s_addr_str = inet_addr_resolve(s_info->ai_addr);
  if (!s_addr_str) {
    loge_errno("inet_ntop failure");
    goto_rc(rc, EXIT_FAILURE, __return_rc);
  }
  logi("using: %s:%d", s_addr_str, s_port);
  free(s_addr_str);

  int s_sock = socket(s_info->ai_family, s_info->ai_socktype, s_info->ai_protocol);
  if (s_sock < 0) {
    loge_errno("socket failure");
    goto_rc(rc, EXIT_FAILURE, __freeaddrinfo_s_info);
  }
  logi("socket success...");

  int optval = 1;
  rc = setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (rc < 0) {
    loge_errno("setsockopt failure");
    goto_rc(rc, EXIT_FAILURE, __close_s_sock);
  }
  logi("setsockopt success...");

  rc = bind(s_sock, s_info->ai_addr, s_info->ai_addrlen);
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

  struct sockaddr_storage c_addr_storage    = {};
  socklen_t               c_addr_storage_sz = sizeof(c_addr_storage);
  while (1) {
    logi("waiting for connection...");
    int c_sock = accept(s_sock, (struct sockaddr *)&c_addr_storage, &c_addr_storage_sz);
    if (c_sock < 0) {
      loge_errno("accept failure");
      break;
    }
    u16    c_port     = inet_port_resolve(&c_addr_storage);
    char * c_addr_str = inet_addr_resolve(&c_addr_storage);
    logi("established connection: %s:%d", c_addr_str, c_port);
    free(c_addr_str);

    x_tcp_frame_t req = {};
    x_tcp_frame_t rsp = {};
    while (1) {
      x_tcp_frame_zero(&req);
      x_tcp_frame_zero(&rsp);

      rsp.head.op_type = X_TCP_OP_TYPE_RSP;
      if (x_tcp_frame_recv(c_sock, &req) < 0) {
        loge_errno("x_tcp_frame_recv failure");
        break;
      }

#define __x_tcp_op_case(_code, _func) case _code: _func(&req, &rsp); break
      switch ((short)req.head.op_code) {
        __x_tcp_op_case(X_TCP_OP_PING, __x_tcp_op_ping);
        default: {
          x_tcp_error_any(&rsp, X_TCP_ERROR_NOP, "unknown operation: %u", req.head.op_code);
          break;
        }
      }
#undef __x_tcp_op_case

      if (x_tcp_frame_send(c_sock, &rsp) < 0) {
        loge_errno("x_tcp_frame_recv failure");
        break;
      }
    }
    x_tcp_frame_dispose(&req);
    x_tcp_frame_dispose(&rsp);
    close(c_sock);
  }

__close_s_sock:
  close(s_sock);
__freeaddrinfo_s_info:
  freeaddrinfo(s_info);
__return_rc:
  return rc;
}