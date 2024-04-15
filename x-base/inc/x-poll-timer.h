#ifndef __X_POLL_TIMER_H__
#define __X_POLL_TIMER_H__

#include "x-poll.h"
#include <sys/time.h>

int
timer_pollfd_init(pollfd_t *pfd, time_t delay_sec, time_t interval_sec, int flags);

int
timer_pollfd_call(pollfd_t *pfd, uint64_t *exp);

#endif//__X_POLL_TIMER_H__
