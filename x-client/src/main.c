#define _GNU_SOURCE // getaddrinfo, strsignal, etc.

#include "x-client-args.h"
#include "x-server-api.h"
#include "x-server-api-xchg.h"
#include "x-logs.h"
#include "x-misc.h"
#include "x-inet.h"
#include "x-poll.h"
#include "x-poll-signal.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

typedef enum {
	CXS_WAIT = 0,
	CXS_RECV = 1,
	CXS_SEND = 2,
} client_xchg_state_t;

static int
timer_pollfd_init(pollfd_t *pfd, time_t delay_sec, time_t interval_sec) {
	assert(pfd);

	struct timespec tsnow = {};
	if (clock_gettime(CLOCK_REALTIME, &tsnow) < 0) {
		loge_errno("[timer pollfd] clock_gettime failure");
		return -1;
	}
	struct itimerspec itspec = {};
	itspec.it_value.tv_sec = tsnow.tv_sec + delay_sec;
	itspec.it_value.tv_nsec = tsnow.tv_nsec;
	itspec.it_interval.tv_sec = interval_sec;
	itspec.it_interval.tv_nsec = 0;

	int tmrfd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);
	if (tmrfd < 0) {
		loge_errno("[timer pollfd] timerfd_create failure");
		return -1;
	}
	if (timerfd_settime(tmrfd, TFD_TIMER_ABSTIME, &itspec, NULL) < 0) {
		loge_errno("[timer pollfd] timerfd_settime failure");
		close(tmrfd);
		return -1;
	}
	pfd->fd = tmrfd;
	pfd->events = POLLIN;
	pfd->revents = 0;
	return 0;
}

static int
timer_pollfd_call(pollfd_t *pfd, u64 *exp) {
	assert(pfd);
	if (pfd->revents & POLLERR) {
		loge("[timer pollfd] POLLERR");
		return -1;
	}
	if (pfd->revents & POLLHUP) {
		loge("[timer pollfd] POLLHUP");
		return -1;
	}
	if (!(pfd->revents & POLLIN)) {
		return 0;
	}
	if (read(pfd->fd, exp, sizeof(*exp)) != sizeof(*exp)) {
		loge_errno("[timer pollfd] read failure");
		return -1;
	}
	return 1;
}

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
		if (fd_flag_add(fd, O_NONBLOCK) < 0) {
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

	pollfd_t *sigpfd = pollfd_pool_borrow(pfdpool);
	if ((signal_pollfd_init(sigpfd, SIG_BLOCK, -1, O_NONBLOCK, SIGINT, SIGTERM)) < 0) {
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

	server_frame_xchg_t req = {
		.ctx.pfd = clipfd,
		.frame = server_frame_req(SOPC_MESSAGE)
	};
	server_frame_body_set(&req.frame, message, message_sz);
	server_frame_xchg_send_prepare(&req);

	server_frame_xchg_t rsp = {
		.ctx.pfd = clipfd
	};

	pollfd_t *tmrpfd = pollfd_pool_borrow(pfdpool);
	if ((timer_pollfd_init(tmrpfd, 0, args->timeout) < 0)) {
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
			case  0: break;
			case  1: goto __sig;
			default: goto __end1;
		}

		if (state == CXS_WAIT) {
			switch (timer_pollfd_call(tmrpfd, &tmrexp)) {
				case  0: break;
				case  1: {
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
				switch (server_frame_xchg_send(&req)) {
					case SXR_SUCCESS: {
						server_frame_xchg_recv_prepare(&rsp);
						state = CXS_RECV;
						logi("done");
						__fallthrough;
					}
					case SXR_PARTIAL: __fallthrough;
					case SXR_IGNORED: goto __next;
					default: goto __end1;
				}
			}
			case CXS_RECV: {
				switch (server_frame_xchg_recv(&rsp)) {
					case SXR_SUCCESS: {
						if (rsp.frame.head.op_flag & SOPF_FAILURE) {
							server_error_t * err = rsp.frame.body;
							loge("server error [%u][%u]: %s", err->auth, err->code, err->text);
							goto __end1;
						}
						server_frame_xchg_send_prepare(&req);
						state = CXS_WAIT;
						logi("wait");
						__fallthrough;
					}
					case SXR_PARTIAL: __fallthrough;
					case SXR_IGNORED: goto __next;
					default: goto __end1;
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
	server_frame_xchg_disp(&req);
__end0:
	pollfd_pool_free(pfdpool);
	return 1;
}
