#ifndef __X_MISC_H__
#define __X_MISC_H__

#include "x-types.h"

#define arr_sz(x) (sizeof(x) / sizeof(0[x]))
#define arrlen(x) (arr_sz(x) - 1)

#define goto_rc(_rf, _rv, _l) ((_rf) = (_rv)); goto _l

char * x_hexdump(const void * src, size_t src_sz);

#endif//__X_MISC_H__