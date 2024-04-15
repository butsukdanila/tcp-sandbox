#define _GNU_SOURCE // sigaddset

#include "x-poll-signal.h"
#include "x-logs.h"

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

int
signal_pollfd_init(pollfd_t *pfd, int how, int fd, int flags, /*signos*/ ...) {
	assert(pfd);

	sigset_t sigset = {};
	va_list signos;
	va_start(signos, flags);
	int signo = 0;
	while ((signo = va_arg(signos, int))) {
		sigaddset(&sigset, signo);
	}
	va_end(signos);
	if (sigprocmask(how, &sigset, null) < 0) {
		loge_errno("[signal pollfd] sigprocmasc failure");
		return -1;
	}

	int sigfd = signalfd(fd, &sigset, flags);
	if (sigfd < 0) {
		loge_errno("[signal pollfd] signalfd failure");
		return -1;
	}
	pfd->fd = sigfd;
	pfd->events = POLLIN;
	pfd->revents = 0;
	return 0;
}

int
signal_pollfd_call(const pollfd_t *pfd, signalfd_siginfo_t *siginfo) {
	assert(pfd);
	assert(siginfo);
	if (pfd->revents & POLLERR) {
		loge("[signal pollfd] POLLERR");
		return -1;
	}
	if (pfd->revents & POLLHUP) {
		loge("[signal pollfd] POLLHUP");
		return -1;
	}
	if (!(pfd->revents & POLLIN)) {
		return 0;
	}
	if (read(pfd->fd, siginfo, sizeof(*siginfo)) != sizeof(*siginfo)) {
		loge_errno("[signal pollfd] read failure");
		return -1;
	}
	return 1;
}