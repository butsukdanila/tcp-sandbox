#include "x-inet.h"
#include "x-logs.h"
#include "x-util.h"

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