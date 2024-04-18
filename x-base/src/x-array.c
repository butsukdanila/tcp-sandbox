#include "x-array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define __array_off(_arr, _idx) \
  ((_arr)->cfg.obj_size * (size_t)(_idx))

#define __array_get(_arr, _idx) \
  ((_arr)->buf + __array_off(_arr, _idx))

typedef struct {
  void       *buf;
  size_t      len;
  arraycfg_t  cfg;
} __array_ext_t;

static void
__array_maybe_extend(__array_ext_t *__arr, size_t extension) {
  size_t req = __arr->len + extension;
  if (req <= __arr->cfg.cap) {
    return;
  }
  __arr->cfg.cap = req;
  __arr->buf = realloc(
    __arr->buf,
    __array_off(__arr, __arr->cfg.cap)
  );
}

array_t *
array_new(arraycfg_t cfg) {
  __array_ext_t *ret = malloc(sizeof(__array_ext_t));
  ret->buf = null;
  ret->len = 0;
  ret->cfg.cap = 0;
  ret->cfg.obj_size = cfg.obj_size;
  ret->cfg.obj_disp = cfg.obj_disp;
  __array_maybe_extend(ret, cfg.cap);
  return (array_t *)ret;
}

void
array_grow_ext(array_t *arr, size_t extension) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  __array_maybe_extend(__arr, extension);
}

void
array_grow(array_t *arr) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  __array_maybe_extend(__arr, __arr->cfg.cap * 2);
}

void *
array_free_ext(array_t *arr, bool extract) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  void *ret = __arr->buf;
  if (!extract) {
    if (__arr->cfg.obj_disp) {
      for (size_t i = 0; i < __arr->len; ++i) {
        __arr->cfg.obj_disp(__array_get(__arr, i));
      }
    }
    free(__arr->buf);
    ret = null;
  }
  free(__arr);
  return ret;
}

void
array_free(array_t *__arr) {
  array_free_ext(__arr, false);
}

void *
array_get(array_t *arr, size_t idx) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  assert(__arr->len > idx);
  return __array_get(__arr, idx);
}

void *
array_set(array_t *arr, size_t idx, void *objs, size_t objs_len) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  assert(__arr->len > idx + objs_len);
  return memcpy(
    __array_get(__arr, idx),
    objs,
    __array_off(__arr, objs_len)
  );
}

void *
array_add(array_t *arr, void *objs, size_t objs_len) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  __array_maybe_extend(__arr, objs_len);
  void *ret = memcpy(
    __array_get(__arr, __arr->len),
    objs,
    __array_off(__arr, objs_len)
  );
  __arr->len += objs_len;
  return ret;
}

void *
array_req(array_t *arr) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  __array_maybe_extend(__arr, 1);
  void * ret = __array_get(__arr, __arr->len);
  __arr->len++;
  return memset(ret, 0, __array_off(__arr, 1));
}

void
array_del(array_t *arr, size_t idx) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  assert(__arr->len > idx);

  if (__arr->cfg.obj_disp) {
    __arr->cfg.obj_disp(__array_get(__arr, idx));
  }

  if (idx != __arr->len - 1) {
    memmove(
      __array_get(__arr, idx),
      __array_get(__arr, idx + 1),
      __array_off(__arr, __arr->len - idx - 1)
    );
  }
  __arr->len--;
}

void
array_del_fast(array_t *arr, size_t idx) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  assert(__arr->len < idx);

  if (__arr->cfg.obj_disp) {
    __arr->cfg.obj_disp(__array_get(__arr, idx));
  }
  if (idx != __arr->len - 1) {
    memcpy(
      __array_get(__arr, idx),
      __array_get(__arr, __arr->len - 1),
      __array_off(__arr, 1)
    );
  }
  __arr->len--;
}

bool
array_capped(array_t *arr) {
  __array_ext_t *__arr = (__array_ext_t *)arr;
  return __arr->len == __arr->cfg.cap;
}