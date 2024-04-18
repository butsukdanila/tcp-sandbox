#include "x-server.h"
#include "x-server-args.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-poll-pool.h"
#include "x-poll-signal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv) {
  server_args_t *args = &server_args_default();
  server_args_parse(argc, argv, args);

  pollfd_pool_t *pfdpool = pollfd_pool_new(
    (/*signal pollfd*/ 1) +
    (/*server pollfd*/ 1) +
    (args->clients_cap ? args->clients_cap : (
      logi("clients capacity can't be zero. defaulting to: "tostr(CLIENTS_CAP_DEF)"..."),
      (size_t)(CLIENTS_CAP_DEF)
    ))
  );

  sigset_t sigset = {};
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGKILL);
  pollfd_t *sigpfd = pollfd_pool_borrow(pfdpool);
  if ((signal_pollfd_init(sigpfd, SIG_BLOCK, &sigset, -1, O_NONBLOCK)) < 0) {
    goto __end;
  }
  signalfd_siginfo_t siginf = {};

  server_t *server = server_init(args, pfdpool);
  if (!server) {
    goto __end;
  }

  while (1) {
    if (pollfd_pool_poll(pfdpool, -1) < 0) {
      goto __end;
    }

    switch (signal_pollfd_call(sigpfd, &siginf)) {
      case IGNORED: break;
      case SUCCESS: goto __sig;
      default:      goto __end;
    }

    switch (server_call(server)) {
      case IGNORED: __fallthrough;
      case SUCCESS: break;
      default:      goto __end;
    }
  }
__sig:
  logi("signal pollfd captured: %s (%d)",
    strsignal((i32)siginf.ssi_signo), (i32)siginf.ssi_signo);

__end:
  server_free(server);
  pollfd_pool_free(pfdpool);
  return 1;
}