#include "x-util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t
str_sz(char *str) {
  return str ? strlen(str) + (/*null*/ 1) : 0;
}

char *
vstrfmt(char *fmt, va_list args) {
  va_list argscp;
  va_copy(argscp, args);
  size_t ret_sz = (size_t)(vsnprintf(NULL, 0, fmt, argscp) + (/*null*/ 1));
  va_end(argscp);
  char *ret = malloc(ret_sz);
  vsnprintf(ret, ret_sz, fmt, args);
  return ret;
}

char *
strfmt(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char *ret = vstrfmt(fmt, args);
  va_end(args);
  return ret;
}

bool
fd_hasfl(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFL);
  return fdf >= 0 ? fdf & flag : false;
}

int
fd_addfl(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFL);
  return fdf >= 0 ? fcntl(fd, F_SETFL, fdf | flag) : -1;
}

int
fd_remfl(int fd, int flag) {
  int fdf = fcntl(fd, F_GETFD);
  return fdf >= 0 ? fcntl(fd, F_SETFL, fdf & ~flag) : -1;
}