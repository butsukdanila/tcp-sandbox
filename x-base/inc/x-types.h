#ifndef __X_TYPES_H__
#define __X_TYPES_H__

#include <bits/wordsize.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef          char i08;
typedef signed   char s08;
typedef unsigned char u08;

typedef          short i16;
typedef signed   short s16;
typedef unsigned short u16;

typedef          int i32;
typedef signed   int s32;
typedef unsigned int u32;

#if __WORDSIZE == 64
typedef          long int i64;
typedef signed   long int s64;
typedef unsigned long int u64;
#else
typedef          long long int i64;
typedef signed   long long int s64;
typedef unsigned long long int u64;
#endif

#define null (void *)0

#define container_of(ptr, type, member) \
  ((type *)((void *)(ptr) - offsetof(type, member)))

#define __unused      __attribute__((unused))
#define __fallthrough __attribute__((fallthrough))

typedef enum {
  RUPTURE = -2,
  FAILURE = -1,
  SUCCESS =  0,
  IGNORED =  1,
  PARTIAL =  2,
} result_t;

#endif//__X_TYPES_H__