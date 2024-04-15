#ifndef __X_ARRAY_H__
#define __X_ARRAY_H__

#include "x-types.h"
#include "x-funcs.h"

struct array {
  void *buf;
  size_t len;
};
typedef struct array array_t;

struct arraycfg {
  size_t cap;
  size_t obj_size;
  mutate_f obj_disp;
};
typedef struct arraycfg arraycfg_t;

array_t *
array_new(arraycfg_t cfg);

void
array_grow_ext(array_t *arr, size_t extension);

void
array_grow(array_t *arr);

void *
array_free_ext(array_t *arr, bool extract);

void
array_free(array_t *arr);

void *
array_get(array_t *arr, size_t idx);

void *
array_set(array_t *arr, size_t idx, void *objs, size_t objs_len);

void *
array_add(array_t *arr, void *objs, size_t objs_len);

void *
array_req(array_t *arr);

void
array_del(array_t *arr, size_t idx);

void
array_del_fast(array_t *arr, size_t idx);

bool
array_capped(array_t *arr);

#define array_set_1(_arr, _idx, _val) \
  array_set((_arr), (size_t)(_idx), (_val), 1)

#define array_add_1(_arr, _val) \
  array_add((_arr), (_val), 1)

#endif//__X_ARRAY_H__