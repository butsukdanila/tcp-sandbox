#define _GNU_SOURCE // getaddrinfo etc.

#include "x-inet.h"
#include "x-misc.h"
#include "x-logs.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *
sockaddr_in_addr(const void *__sa) {
  return sockaddr_family_ternary(__sa,
    (void *)&as_sockaddr_in(__sa)->sin_addr,
    (void *)&as_sockaddr_in6(__sa)->sin6_addr,
    (void *)0
  );
}

in_port_t
sockaddr_in_port(const void *__sa) {
  return (in_port_t)sockaddr_family_ternary(__sa,
    as_sockaddr_in(__sa)->sin_port,
    as_sockaddr_in6(__sa)->sin6_port,
    0
  );
}

socklen_t
sockaddr_in_addrstrlen(const void *__sa) {
  return (socklen_t)sockaddr_family_ternary(__sa,
    INET_ADDRSTRLEN,
    INET6_ADDRSTRLEN,
    0
  );
}

char *
sockaddr_in_addrstr(const void *__sa) {
  socklen_t len = sockaddr_in_addrstrlen(__sa);
  if (!len) {
    return null;
  }
  return (char *)inet_ntop(
    sockaddr_family(__sa),
    sockaddr_in_addr(__sa),
    malloc(len),
    len
  );
}

int
socket_ext(const char * address, const char * port, int flags) {
	logi("[socket] address info...");
	addrinfo_t *ai_hint = &(addrinfo_t) {
		.ai_family   = PF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags    = AI_PASSIVE
	};
	addrinfo_t *ai = null;
	int rc = getaddrinfo(address, port, ai_hint, &ai);
	if (rc < 0) {
		loge("[socket] getaddrinfo failure: %s (%d)", gai_strerror(rc), rc);
		return -1;
	}

	logi("[socket] probing...");
	int fd = -1;
	for (addrinfo_t *p = ai; p; p = p->ai_next) {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			loge_errno("[socket] failure");
			continue;
		}
		if (fd_flag_add(fd, flags) < 0) {
			loge_errno("[socket] fd add flag failure");
			goto __probe_next;
		}
		if (bind(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
			loge_errno("[socket] bind failure");
			goto __probe_next;
		}
		// success
		break;

	__probe_next:
		close(fd);
		fd = -1;
	}
	freeaddrinfo(ai);
	return fd;
}

static int
getnameany(
	int fd, char ** addr, in_port_t * port, 
	int (* call)(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __len)
) {
	sockaddr_storage_t sa_storage    = {};
	socklen_t          sa_storage_sz = sizeof(sa_storage);
	if (call(fd, as_sockaddr(&sa_storage), &sa_storage_sz) < 0) {
		return -1;
	}
	*addr = sockaddr_in_addrstr(&sa_storage);
	*port = sockaddr_in_port(&sa_storage);
	return 0;
}

int
getpeernamestr(int fd, char ** addr, in_port_t * port) {
	return getnameany(fd, addr, port, getpeername);
}

int
getsocknamestr(int fd, char ** addr, in_port_t * port) {
	return getnameany(fd, addr, port, getsockname);
}