#ifndef __X_POLL_TIMER_H__
#define __X_POLL_TIMER_H__

#include "x-poll.h"
#include <sys/time.h>
#include <sys/timerfd.h>

typedef struct itimerspec itimerspec_t;

int
timer_pollfd_init(pollfd_t *pfd, time_t delay_sec, time_t interval_sec, int flags);

int
timer_pollfd_read(pollfd_t *pfd, uint64_t *exp);

int
timer_pollfd_arm(pollfd_t *pfd, itimerspec_t *utmr);

int
timer_pollfd_disarm(pollfd_t *pfd);

#endif//__X_POLL_TIMER_H__