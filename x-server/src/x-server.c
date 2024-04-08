#define _GNU_SOURCE // getaddrinfo etc.

#include "x-server-api.h"
#include "x-server-args.h"
#include "x-server-impl.h"

#include "x-logs.h"
#include "x-misc.h"
#include "x-inet.h"
#include "x-array.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/signalfd.h>

typedef struct signalfd_siginfo siginf_t;

static
void
xs_pollfd_disp(void * obj) {
  pollfd_t * pfd = obj;
  if (!pfd) return;
  close(pfd->fd);
}

static
void
xs_pollfds_revents_reset(x_array_t * pfds) {
  for (size_t i = 0; i < pfds->len; ++i) {
    pollfd_t * pfd = x_array_get(pfds, i);
    pfd->revents = 0;
  }
}

static
int
xs_sigpfd_handle(pollfd_t * sigpfd, siginf_t * siginf) {
  if (sigpfd->revents & POLLERR) {
    loge("signal pollfd error");
    return -1;
  }
  if (sigpfd->revents & POLLIN) {
    logi("signal pollfd reading...");
    if (read(sigpfd->fd, siginf, sizeof(*siginf)) != sizeof(*siginf)) {
      loge_errno("signal pollfd read failure");
      return -1;
    }
    return 1;
  }
  return 0;
}

static
int
xs_srvpfd_handle(pollfd_t * srvpfd, x_array_t * pfds, x_array_t * actors) {
  if (srvpfd->revents & POLLERR) {
    loge("server pollfd error");
    return -1;
  }

  if (srvpfd->revents & POLLIN) {
    logi("server pollfd accepting client...");
    int actor_fd = accept(srvpfd->fd, NULL, NULL);
    if (actor_fd < 0) {
      loge_errno("client accept failure");
      return -1;
    }
    logi("client socket unblock...");
    if (x_fd_add_flag(actor_fd, O_NONBLOCK) < 0) {
      loge_errno("client socket unblock failure");
      return -1;
    }
    logi("client socket retrieving address info...");
    sockaddr_storage_t sa_storage = {};
    socklen_t          sa_storage_sz = sizeof(sa_storage);
    if (getpeername(actor_fd, as_sockaddr(&sa_storage), &sa_storage_sz) < 0) {
      loge_errno("client socket getsockname");
      return -1;
    }

    pollfd_t * actor_pfd = x_array_add_1(pfds, malloc(sizeof(*actor_pfd)));
    actor_pfd->fd      = actor_fd;
    actor_pfd->events  = 0;
    actor_pfd->revents = 0;
    x_array_add_1(actors, xs_actor_init(actor_pfd));

    char *    in_addr = sockaddr_in_addrstr(&sa_storage);
    in_port_t in_port = sockaddr_in_port(&sa_storage);
    logi("client socket ready: %s:%d", in_addr, ntohs(in_port));
    free(in_addr);
    return 1;
  }
  return 0;
}

static
int
xs_actors_handle(x_array_t * pfds, x_array_t * actors) {
  bool slimed = false;
  for (size_t i = 0; i < actors->len; ++i) {
    xs_actor_t * actor = x_array_get(actors, i);
    int rc = xs_actor_call(actor);
    if (rc < 0) {
      x_array_del_fast(actors, i);
      x_array_del_fast(pfds, i + 2);
      slimed = true;
    }
  }
  return slimed ? 1 : 0;
}

int
main(int argc, char ** argv) {
  xs_args_t args = xs_args_default();
  xs_args_parse(argc, argv, &args);

  x_array_t * pfds = x_array_init(sizeof(pollfd_t), xs_pollfd_disp, (size_t)(args.clients_size) + 2);
  x_array_t * actors = x_array_init(xs_actor_size(), xs_actor_disp, (size_t)(args.clients_size));

  logi("signal fd setup...");
  pollfd_t * sigpfd = x_array_add_1(pfds, malloc(sizeof(*sigpfd)));
  siginf_t   siginf = {};
  sigset_t   sigset = {};
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGKILL);
  if (sigprocmask(SIG_SETMASK, &sigset, NULL) < 0) {
    loge_errno("sigprocmasc failure");
    goto __end;
  }
  if ((sigpfd->fd = signalfd(-1, &sigset, 0)) < 0) {
    loge_errno("signalfd failure");
    goto __end;
  }
  sigpfd->events = POLLIN;

  logi("server fd setup...");
  pollfd_t * srvpfd = x_array_add_1(pfds, malloc(sizeof(*srvpfd)));
  if ((srvpfd->fd = xs_socket(args.address, args.port, args.backlog)) < 0) {
    loge("xs_socket failure");
    goto __end;
  }
  srvpfd->events = POLLIN;

  while (1) {
    xs_pollfds_revents_reset(pfds);
    if (poll(pfds->buf, pfds->len, -1) < 0) {
      loge_errno("poll failure");
      goto __end;
    }

    switch (xs_sigpfd_handle(sigpfd, &siginf)) {
      case  1: goto __sig;    // signal received
      case  0: break;         // ignored
      case -1: __fallthrough; // failure
      default: goto __end;    // unknown
    }

    bool growed = false;
    switch (xs_srvpfd_handle(srvpfd, pfds, actors)) {
      case  1: growed = true; // actor created
      case  0: break;         // ignored
      case -1: __fallthrough; // failure
      default: goto __end;    // unknown
    }
    if (growed && pfds->len == pfds->cap) {
      logi("clients capacity reached...");
      if (args.clients_grow) {
        logi("clients capacity grow...");
        x_array_grow(pfds);
        x_array_grow(actors);
      } else {
        logi("server fd disabled...");
        srvpfd->fd = -srvpfd->fd;
      }
    }

    bool slimed = false;
    switch (xs_actors_handle(pfds, actors)) {
      case  1: slimed = true; // actor removed
      case  0: break;         // ignored
      default: goto __end;    // unknown
    }
    if (slimed && !args.clients_grow) {
      logi("server fd enabled...");
      sigpfd->fd = -sigpfd->fd;
    }
  }

__sig:
  logi(
    "signal received: %s (%d)",
    strsignal((i32)siginf.ssi_signo), (i32)siginf.ssi_signo
  );

__end:
  x_array_free(actors, false);
  x_array_free(pfds, false);
  return 0;
}