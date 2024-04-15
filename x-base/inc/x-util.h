#ifndef __X_UTIL_H__
#define __X_UTIL_H__

#include "x-types.h"

#include <stdarg.h>
#include <sys/fcntl.h>

#define quote(x) #x
#define tostr(x) quote(x)

#define arr_sz(x) (sizeof(x) / sizeof(0[x]))
#define arrlen(x) (arr_sz(x) - 1)

size_t
str_sz(char *str);

char *
vstrfmt(char *fmt, va_list args);

char *
strfmt(char *fmt, ...);

bool
fd_hasfl(int fd, int flag);

int
fd_addfl(int fd, int flag);

int
fd_remfl(int fd, int flag);

#endif//__X_UTIL_H__
