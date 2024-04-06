#ifndef __X_UTILS_H__
#define __X_UTILS_H__

#include <x-types.h>

#define arr_sz(x) (sizeof(x) / sizeof(0[x]))
#define arrlen(x) (arr_sz(x) - 1)

char * x_hexdump(const void * src, size_t src_sz);

#endif//__X_UTILS_H__