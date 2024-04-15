#ifndef __X_INET_H__
#define __X_INET_H__

#include "x-types.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>

typedef struct sockaddr         sockaddr_t;
typedef struct sockaddr_in      sockaddr_in_t;
typedef struct sockaddr_in6     sockaddr_in6_t;
typedef struct addrinfo         addrinfo_t;
typedef struct sockaddr_storage sockaddr_storage_t;

#define as_sockaddr(x)     ((sockaddr_t     *)(x))
#define as_sockaddr_in(x)  ((sockaddr_in_t  *)(x))
#define as_sockaddr_in6(x) ((sockaddr_in6_t *)(x))

#define sockaddr_family(x) \
  as_sockaddr(__sa)->sa_family

#define sockaddr_family_ternary(x, in, in6, def) ({ \
  sa_family_t sa_family = sockaddr_family(x);       \
  sa_family == AF_INET  ? (in)  :                   \
  sa_family == AF_INET6 ? (in6) : (def);            \
})

void *
sockaddr_in_addr(const void *__sa);

in_port_t
sockaddr_in_port(const void *__sa);

socklen_t
sockaddr_in_addrstrlen(const void *__sa);

char *
sockaddr_in_addrstr(const void *__sa);

int
getpeernamestr(int fd, char ** addr, in_port_t * port);

int
getsocknamestr(int fd, char ** addr, in_port_t * port);

#endif//__X_INET_H__