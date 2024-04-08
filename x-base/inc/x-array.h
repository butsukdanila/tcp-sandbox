#ifndef __X_ARRAY_H__
#define __X_ARRAY_H__

#include "x-types.h"
#include "x-funcs.h"

struct __array {
  void * buf;
  size_t len;
  size_t cap;
  size_t obj_size;
};
typedef struct __array x_array_t;

x_array_t *
x_array_init(size_t obj_size, x_mut_f obj_free, size_t cap);

void
x_array_grow(x_array_t * arr);

void *
x_array_free(x_array_t * arr, bool extract);

void *
x_array_set(x_array_t * arr, size_t idx, void * objs, size_t objs_len);

void *
x_array_add(x_array_t * arr, void * objs, size_t objs_len);

void
x_array_del(x_array_t * arr, size_t idx);

void
x_array_del_fast(x_array_t * arr, size_t idx);

#define x_array_off(_arr, _idx) \
  ((_arr)->obj_size * (size_t)(_idx))

#define x_array_get(_arr, _idx) \
  ((_arr)->buf + x_array_off(_arr, _idx))

#define x_array_set_1(_arr, _idx, _val) \
  x_array_set((_arr), (size_t)(_idx), (_val))

#define x_array_add_1(_arr, _val) \
  x_array_add((_arr), (_val), 1)

#endif//__X_ARRAY_H__