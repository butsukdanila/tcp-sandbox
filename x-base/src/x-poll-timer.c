#include "x-poll-timer.h"
#include "x-logs.h"

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/fcntl.h>

int
timer_pollfd_init(pollfd_t *pfd, time_t delay_sec, time_t interval_sec, int flags) {
  struct timespec tsnow = {};
  if (clock_gettime(CLOCK_REALTIME, &tsnow) < 0) {
    loge_errno("[timer pollfd] clock_gettime failure");
    return FAILURE;
  }

  int tmrfd = timerfd_create(CLOCK_REALTIME, flags);
  if (tmrfd < 0) {
    loge_errno("[timer pollfd] timerfd_create failure");
    return FAILURE;
  }
  pfd->fd = tmrfd;

  itimerspec_t itspec = {};
  itspec.it_value.tv_sec = tsnow.tv_sec + delay_sec;
  itspec.it_value.tv_nsec = tsnow.tv_nsec;
  itspec.it_interval.tv_sec = interval_sec;
  itspec.it_interval.tv_nsec = 0;
  if (timer_pollfd_arm(pfd, &itspec) < 0) {
    close(pfd->fd);
    return FAILURE;
  }
  return SUCCESS;
}

int
timer_pollfd_read(pollfd_t *pfd, uint64_t *exp) {
  if (pfd->revents & POLLERR) {
    loge("[timer pollfd] POLLERR");
    return FAILURE;
  }
  if (pfd->revents & POLLHUP) {
    loge("[timer pollfd] POLLHUP");
    return RUPTURE;
  }
  if (!(pfd->revents & POLLIN)) {
    return IGNORED;
  }
  if (read(pfd->fd, exp, sizeof(*exp)) != sizeof(*exp)) {
    loge_errno("[timer pollfd] read failure");
    return FAILURE;
  }
  return SUCCESS;
}

int
timer_pollfd_arm(pollfd_t *pfd, itimerspec_t *utmr) {
  if (timerfd_settime(pfd->fd, TFD_TIMER_ABSTIME, utmr, NULL) < 0) {
    loge_errno("[timer pollfd] timerfd_settime failure");
    return FAILURE;
  }
  pfd->events = POLLIN;
  pfd->revents = 0;
  return SUCCESS;
}

int
timer_pollfd_disarm(pollfd_t *pfd) {
  itimerspec_t itspec = {};
  if (timerfd_settime(pfd->fd, TFD_TIMER_ABSTIME, &itspec, NULL) < 0) {
    loge_errno("[timer pollfd] timerfd_settime failure");
    return FAILURE;
  }
  pfd->events = 0;
  pfd->revents = 0;
  return SUCCESS;
}