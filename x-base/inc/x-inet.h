#ifndef __X_INET_H__
#define __X_INET_H__

#include "x-types.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>

typedef struct sockaddr     sa_t;
typedef struct sockaddr_in  sa_in4_t;
typedef struct sockaddr_in6 sa_in6_t;
typedef socklen_t           sl_t;

u16    sa_port(const void * __sa);
char * sa_addr_str(const void * __sa);

#if _POSIX_C_SOURCE >= 200112L
typedef struct addrinfo         ai_t;
typedef struct sockaddr_storage sa_storage_t;
#endif

#endif//__X_INET_H__