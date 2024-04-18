#include "x-server.h"
#include "x-server-args.h"
#include "x-logs.h"
#include "x-util.h"
#include "x-poll-pool.h"
#include "x-poll-signal.h"

#include <stdlib.h>

int
main(int argc, char **argv) {
  int rc = EXIT_SUCCESS;

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
  if ((signal_pollfd_init(sigpfd, SIG_BLOCK, &sigset, -1, SFD_NONBLOCK)) < 0) {
    rc = EXIT_FAILURE;
    goto __pollfd_free;
  }
  signalfd_siginfo_t siginf = {};

  server_t *server = server_new(args, pfdpool);
  if (!server) {
    rc = EXIT_FAILURE;
    goto __pollfd_free;
  }

  while (1) {
    if (pollfd_pool_poll(pfdpool, -1) < 0) {
      rc = EXIT_FAILURE;
      goto __server_free;
    }

    switch (signal_pollfd_read(sigpfd, &siginf)) {
      case IGNORED: break;
      case SUCCESS: {
        rc = EXIT_SUCCESS;
        goto __signal_read;
      }
      default: {
        rc = EXIT_FAILURE;
        goto __server_free;
      }
    }

    switch (server_xchg(server)) {
      case IGNORED: __fallthrough;
      case SUCCESS: break;
      default: {
        rc = EXIT_FAILURE;
        goto __server_free;
      }
    }
  }

__signal_read:
  logi("signal pollfd captured: %s (%d)",
    strsignal((i32)siginf.ssi_signo), (i32)siginf.ssi_signo);

__server_free:
  server_free(server);
__pollfd_free:
  pollfd_pool_free(pfdpool);
  return rc;
}