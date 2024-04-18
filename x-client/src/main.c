#include "x-client.h"
#include "x-logs.h"
#include "x-poll-pool.h"
#include "x-poll-signal.h"

#include <stdlib.h>

int
main(int argc, char **argv) {
  int rc = EXIT_SUCCESS;

  client_args_t *args = &client_args_default();
  client_args_parse(argc, argv, args);

  pollfd_pool_t *pfdpool = pollfd_pool_new(
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
  if ((signal_pollfd_init(sigpfd, SIG_BLOCK, &sigset, -1, SFD_NONBLOCK)) < 0) {
    rc = EXIT_FAILURE;
    goto __pollfd_free;
  }
  signalfd_siginfo_t siginf = {};

  client_t *client = client_new(args, pfdpool);
  if (!client) {
    rc = EXIT_FAILURE;
    goto __pollfd_free;
  }

  while (1) {
    if (pollfd_pool_poll(pfdpool, -1) < 0) {
      rc = EXIT_FAILURE;
      goto __client_free;
    }

    switch (signal_pollfd_read(sigpfd, &siginf)) {
      case IGNORED: break;
      case SUCCESS: {
        rc = EXIT_SUCCESS;
        goto __signal_read;
      }
      default: {
        rc = EXIT_FAILURE;
        goto __client_free;
      }
    }

    switch (rc = client_xchg(client)) {
      case PARTIAL: __fallthrough;
      case IGNORED: break;
      case SUCCESS: {
        rc = EXIT_SUCCESS;
        goto __client_free;
      }
      default: {
        switch (rc) {
          case RUPTURE: logi("rupture"); break;
          case FAILURE: loge("failure"); break;
          default:      loge("unknown"); break;
        }
        rc = EXIT_FAILURE;
        goto __client_free;
      }
    }
  }

__signal_read:
  logi("[signal] captured: %s (%d)",
    strsignal((i32)siginf.ssi_signo), (i32)siginf.ssi_signo);

__client_free:
  client_free(client);
__pollfd_free:
  pollfd_pool_free(pfdpool);
  return rc;
}