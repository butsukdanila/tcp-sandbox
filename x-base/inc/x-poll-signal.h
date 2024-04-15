#ifndef __X_POLL_SIGNAL_H__
#define __X_POLL_SIGNAL_H__

#include "x-poll.h"
#include <sys/signal.h>
#include <sys/signalfd.h>

typedef struct signalfd_siginfo signalfd_siginfo_t;

int
signal_pollfd_init(pollfd_t *pfd, int how, int fd, int flags, /*signos*/ ...);

int
signal_pollfd_call(const pollfd_t *pfd, signalfd_siginfo_t *siginfo);

#endif//__X_POLL_SIGNAL_H__