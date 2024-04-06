#include "x-inet.h"
#include <stdlib.h>

u16 sa_port(const void * __sa) {
  switch (((sa_t *)__sa)->sa_family) {
    case AF_INET:  return ntohs(((sa_in4_t *)__sa)->sin_port);
    case AF_INET6: return ntohs(((sa_in6_t *)__sa)->sin6_port);
  }
  return 0;
}

char * sa_addr_str(const void * __sa) {
  switch (((sa_t *)__sa)->sa_family) {
    case AF_INET: {
      return (char *)inet_ntop(AF_INET, &((sa_in4_t *)__sa)->sin_addr,
        malloc(INET_ADDRSTRLEN), INET_ADDRSTRLEN
      );
    }
    case AF_INET6: {
      return (char *)inet_ntop(AF_INET6, &((sa_in6_t *)__sa)->sin6_addr,
        malloc(INET6_ADDRSTRLEN), INET6_ADDRSTRLEN
      );
    };
    default: {
      return NULL;
    }
  }
}