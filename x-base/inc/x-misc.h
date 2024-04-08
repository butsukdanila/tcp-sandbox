#ifndef __X_MISC_H__
#define __X_MISC_H__

#include "x-types.h"

#define quote(x) #x
#define tostr(x) quote(x)

#define goto_r(_r, _v, _l) ((_r) = (_v)); goto _l

#define arr_sz(x) (sizeof(x) / sizeof(0[x]))

#define __fallthrough __attribute__((fallthrough))
#define __unused      __attribute__((unused))

char *
x_hexdump(const void * src, size_t src_sz);

bool
x_fd_has_flag(int fd, int flag);

int
x_fd_add_flag(int fd, int flag);

int
x_fd_del_flag(int fd, int flag);

#endif//__X_MISC_H__