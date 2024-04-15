#ifndef __X_MISC_H__
#define __X_MISC_H__

#include "x-types.h"
#include <stdarg.h>

#define quote(x) #x
#define tostr(x) quote(x)

#define arr_sz(x) (sizeof(x) / sizeof(0[x]))
#define arrlen(x) (arr_sz(x) - 1)

#define container_of(ptr, type, member) \
	((type *)((void *)(ptr) - offsetof(type, member)))

#define __fallthrough __attribute__((fallthrough))
#define __unused      __attribute__((unused))

char *
hexdump(const void *src, size_t src_sz);

size_t
str_sz(char * str);

char *
vstrfmt(char * fmt, va_list args);

char *
strfmt(char * fmt, ...);

bool
fd_flag_has(int fd, int flag);

int
fd_flag_add(int fd, int flag);

int
fd_flag_del(int fd, int flag);

#endif//__X_MISC_H__