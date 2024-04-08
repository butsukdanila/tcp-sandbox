#include "x-array.h"

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

typedef struct {
  void *  buf;      /* underlying memory buffer */
  size_t  len;      /* effective array length   */
  size_t  cap;      /* current array capacity   */
  size_t  obj_size; /* size of object in memory */
  x_mut_f obj_disp; /* dispose object resources */
} __array_ext_t;

static
void
__array_maybe_expand(__array_ext_t * arr, size_t exp) {
  size_t req = arr->len + exp;
  if (req <= arr->cap) {
    return;
  }
  arr->buf = realloc(
    arr->buf,
    x_array_off(
      arr, 
      arr->cap = req
    )
  );
}

x_array_t *
x_array_init(size_t obj_size, x_mut_f obj_disp, size_t cap) {
  __array_ext_t * ret = malloc(sizeof(__array_ext_t));
  ret->buf      = NULL;
  ret->len      = 0;
  ret->cap      = 0;
  ret->obj_size = obj_size;
  ret->obj_disp = obj_disp;
  if (cap) {
    __array_maybe_expand(ret, cap);
  }
  return (x_array_t *)ret;
}

void
x_array_grow(x_array_t * farr) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  __array_maybe_expand(
    arr,
    arr->cap ? arr->cap * 2 : 1
  );
}

void *
x_array_free(x_array_t * farr, bool extract) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  void * ret = arr->buf;
  if (!extract) {
    if (arr->obj_disp) {
      for (size_t i = 0; i < arr->len; ++i) {
        arr->obj_disp(x_array_get(arr, i));
      }
    }
    free(arr->buf);
    ret = NULL;
  }
  free(arr);
  return ret;
}

void *
x_array_set(x_array_t * farr, size_t idx, void * objs, size_t objs_len) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  assert(objs_len);
  assert(idx + objs_len < arr->len);

  return memcpy(
    x_array_get(arr, idx),
    objs,
    x_array_off(arr, objs_len)
  );
}

void *
x_array_add(x_array_t * farr, void * objs, size_t objs_len) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  assert(objs_len);

  __array_maybe_expand(arr, objs_len);
  void * ret = memcpy(
    x_array_get(arr, arr->len),
    objs,
    x_array_off(arr, objs_len)
  );
  arr->len += objs_len;
  return ret;
}

void
x_array_del(x_array_t * farr, size_t idx) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  assert(idx < arr->len);

  if (arr->obj_disp) {
    arr->obj_disp(x_array_get(arr, idx));
  }
  arr->len -= 1;

  if (idx == arr->len) {
    return;
  }

  memcpy(
    x_array_get(arr, idx),
    x_array_get(arr, idx + 1),
    x_array_off(arr, arr->len)
  );
}

void
x_array_del_fast(x_array_t * farr, size_t idx) {
  __array_ext_t * arr = (__array_ext_t *)farr;
  assert(idx < arr->len);

  if (arr->obj_disp) {
    arr->obj_disp(x_array_get(arr, idx));
  }
  memcpy(
    x_array_get(arr, idx),
    x_array_get(arr, arr->len),
    x_array_off(arr, 1)
  );
  arr->len -= 1;
}