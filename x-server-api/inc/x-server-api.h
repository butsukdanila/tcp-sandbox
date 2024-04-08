#ifndef __X_SERVER_API_H__
#define __X_SERVER_API_H__

#include "x-server-api-defs.h"

#define xs_frame_rsp()          \
  (xs_frame_t) {                \
    .head = {                   \
      .op_type = XS_OP_TYPE_RSP \
    }                           \
  }

#define xs_frame_req(_code)      \
  (xs_frame_t) {                 \
    .head = {                    \
      .op_type = XS_OP_TYPE_REQ, \
      .op_code = _code           \
    }                            \
  }

void
xs_frame_zero(xs_frame_t * frame);

void
xs_frame_disp(xs_frame_t * frame);

void
xs_frame_free(xs_frame_t * frame);

void *
xs_frame_body_realloc(xs_frame_t * frame, u32 body_sz);

void
xs_frame_body_set(xs_frame_t * frame, const void * buf, size_t buf_sz);

int
xs_frame_send(int fd, const xs_frame_t * frame);

int
xs_frame_recv(int fd, xs_frame_t * frame);

int
xs_frame_xchg(int fd, const xs_frame_t * req, xs_frame_t * rsp);

void
xs_error_set0(xs_frame_t * frame, const xs_error_t * err);

void
xs_error_set1(xs_frame_t * frame, u08 auth, u08 code, const char * format, ...);

#define xs_error_any(_frame, _code, _format, ...) \
  xs_error_set1(_frame, XS_ERROR_AUTH_ANY, (u08)_code, _format, ##__VA_ARGS__)

#define xs_error_sys(_frame, _code, _format, ...) \
  xs_error_set1(_frame, XS_ERROR_AUTH_SYS, (u08)_code, _format, ##__VA_ARGS__)

#endif//__X_SERVER_API_H__