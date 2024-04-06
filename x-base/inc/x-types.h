#ifndef __X_TYPES_H__
#define __X_TYPES_H__

#include <bits/wordsize.h>

#include <stddef.h>
#include <stdbool.h>

typedef signed   char s08;
typedef unsigned char u08;

typedef signed   short s16;
typedef unsigned short u16;

typedef signed   int s32;
typedef unsigned int u32;

#if __WORDSIZE == 64
typedef signed   long int s64;
typedef unsigned long int u64;
#else
typedef signed   long long int s64;
typedef unsigned long long int u64;
#endif

#endif//__X_TYPES_H__