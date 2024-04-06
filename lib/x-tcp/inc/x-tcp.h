#ifndef __X_TCP_H__
#define __X_TCP_H__

#include "x-tcp-defs.h"

#define x_tcp_frame_req(_code)      \
  (x_tcp_frame_t) {                 \
    .head = {                       \
      .op_type = X_TCP_OP_TYPE_REQ, \
      .op_code = _code              \
    }                               \
  }

void x_tcp_frame_zero(x_tcp_frame_t * frame);
void x_tcp_frame_dispose(x_tcp_frame_t * frame);
void x_tcp_frame_free(x_tcp_frame_t * frame);

void x_tcp_frame_body_set(x_tcp_frame_t * frame, const void * buf, size_t buf_sz);
#define x_tcp_frame_body_realloc(_frame, _size) \
  (_frame)->body = realloc((_frame)->body, ((_frame)->head.body_sz = (u32)(_size)))

int x_tcp_frame_send(int fd, const x_tcp_frame_t * frame);
int x_tcp_frame_recv(int fd, x_tcp_frame_t * frame);
int x_tcp_frame_xchg(int fd, const x_tcp_frame_t * req, x_tcp_frame_t * rsp);

void x_tcp_error_set0(x_tcp_frame_t * frame, const x_tcp_error_t * err);
void x_tcp_error_set1(x_tcp_frame_t * frame, u08 auth, u08 code, char * tfmt, ...);

#define x_tcp_error_any(_frame, _code, _tfmt, ...) \
  x_tcp_error_set1(_frame, X_TCP_ERROR_AUTH_ANY, (u08)_code, _tfmt, ##__VA_ARGS__)
#define x_tcp_error_sys(_frame, _code, _tfmt, ...) \
  x_tcp_error_set1(_frame, X_TCP_ERROR_AUTH_SYS, (u08)_code, _tfmt, ##__VA_ARGS__)

#endif//__X_TCP_H__