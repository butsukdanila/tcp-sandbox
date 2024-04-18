#include "x-client-args.h"
#include "x-server-api.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-inet.h"
#include "x-poll-pool.h"
#include "x-poll-timer.h"
#include "x-poll-signal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
  CXS_WAIT = 0,
  CXS_RECV = 1,
  CXS_SEND = 2,
} client_xchg_state_t;

static int
client_socket(const char *address, const char *port) {
  addrinfo_t *ai_hint = &(addrinfo_t) {
    .ai_family   = PF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags    = AI_PASSIVE
  };
  addrinfo_t *ai = null;
  int rc = getaddrinfo(address, port, ai_hint, &ai);
  if (rc < 0) {
    loge("client socket getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
    return -1;
  }
  int fd = -1;
  for (addrinfo_t *p = ai; p; p = p->ai_next) {
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) {
      loge_errno("client socket failure");
      continue;
    }
    if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
      loge_errno("client socket connect failure");
      goto __probe_next;
    }
    if (fd_addfl(fd, O_NONBLOCK) < 0) {
      loge_errno("client socket unblock failure");
      goto __probe_next;
    }
    char      *in_addr = null;
    in_port_t  in_port = 0;
    if (getsocknamestr(fd, &in_addr, &in_port) < 0) {
      loge_errno("client socket getpeername failure");
      goto __probe_next;
    }
    logi("client ready: %s:%u", in_addr, ntohs(in_port));
    free(in_addr);
    break;

  __probe_next:
    close(fd);
    fd = -1;
  }
  freeaddrinfo(ai);
  return fd;
}

int
main(int argc, char **argv) {
  client_args_t * args = &client_args_default();
  client_args_parse(argc, argv, args);

  pollfd_pool_t * pfdpool = pollfd_pool_new(
    (/*signal pollfd*/ 1) +
    (/*timer  pollfd*/ 1) +
    (/*client pollfd*/ 1)
  );

  sigset_t sigset = {};
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGKILL);
  pollfd_t *sigpfd = pollfd_pool_borrow(pfdpool);
  if ((signal_pollfd_init(sigpfd, SIG_BLOCK, &sigset, -1, O_NONBLOCK)) < 0) {
    goto __end0;
  }
  signalfd_siginfo_t siginf = {};

  pollfd_t *clipfd = pollfd_pool_borrow(pfdpool);
  clipfd->fd = client_socket(args->address, args->port);
  if (clipfd->fd < 0) {
    goto __end0;
  }

  char * message = args->message ?
    strdup(args->message) :
    strfmt("client[%d]", getpid());
  size_t message_sz = str_sz(message);

  server_xchg_t req = {
    .pfdxg.pfd = clipfd,
    .frame = server_frame_req(SREQ_OP_MESSAGE)
  };
  server_frame_body_set(&req.frame, message, message_sz);
  server_xchg_send_prep(&req);

  server_xchg_t rsp = {
    .pfdxg.pfd = clipfd
  };

  pollfd_t *tmrpfd = pollfd_pool_borrow(pfdpool);
  if ((timer_pollfd_init(tmrpfd, 0, args->timeout, O_NONBLOCK) < 0)) {
    goto __end0;
  }
  u64  tmrexp = 0;
  logi("sending '%s' every %lds...", message, args->timeout);
  int  state = CXS_WAIT;
  while (1) {
    if (pollfd_pool_poll(pfdpool, -1) < 0) {
      goto __end1;
    }

    switch (signal_pollfd_call(sigpfd, &siginf)) {
      case IGNORED: break;
      case SUCCESS: goto __sig;
      default:      goto __end1;
    }

    if (state == CXS_WAIT) {
      switch (timer_pollfd_call(tmrpfd, &tmrexp)) {
        case IGNORED: break;
        case SUCCESS: {
          state = CXS_SEND;
          logi("send");
          break;
        }
        default: goto __end1;
      }
    }
    switch (state) {
      case CXS_WAIT: {
        goto __next;
      }
      case CXS_SEND: {
        switch (server_xchg_send(&req)) {
          case SUCCESS: {
            server_xchg_recv_prep(&rsp);
            state = CXS_RECV;
            logi("done");
            __fallthrough;
          }
          case PARTIAL: __fallthrough;
          case IGNORED: goto __next;
          default:      goto __end1;
        }
      }
      case CXS_RECV: {
        switch (server_xchg_recv(&rsp)) {
          case SUCCESS: {
            if (rsp.frame.head.rsp.status & SRSP_ST_FAILURE) {
              server_error_t * err = rsp.frame.body;
              loge("server error [%u][%u]: %s", err->auth, err->code, err->text);
              goto __end1;
            }
            server_xchg_send_prep(&req);
            state = CXS_WAIT;
            logi("wait");
            __fallthrough;
          }
          case PARTIAL: __fallthrough;
          case IGNORED: goto __next;
          default:      goto __end1;
        }
      }
    }
    loge("something went horribly wrong");
  __next:
  }

__sig:
  logi("[signal] captured: %s (%d)",
    strsignal((i32)siginf.ssi_signo), (i32)siginf.ssi_signo);

__end1:
  free(message);
  server_xchg_disp(&req);
__end0:
  pollfd_pool_free(pfdpool);
  return 1;
}