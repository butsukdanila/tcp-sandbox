#ifndef __X_SERVER_API_H__
#define __X_SERVER_API_H__

#include "x-server-api-defs.h"

#define server_frame() \
  (server_frame_t) {}

#define server_frame_rsp() \
  (server_frame_t) {       \
    .head = {              \
      .op_type = SOPT_RSP  \
    }                      \
  }

#define server_frame_req(_code) \
  (server_frame_t) {            \
    .head = {                   \
      .op_type = SOPT_REQ,      \
      .op_code = _code          \
    }                           \
  }

void
server_frame_zero(server_frame_t *frame);

void
server_frame_disp(server_frame_t *frame);

void
server_frame_free(server_frame_t *frame);

void *
server_frame_body_realloc(server_frame_t *frame, u32 body_sz);

void
server_frame_body_set(server_frame_t *frame, const void *buf, size_t buf_sz);

void
server_error_set0(server_frame_t *frame, const server_error_t *err);

void
server_error_set1(server_frame_t *frame, u16 auth, u16 code, const char *format, ...);

#define server_error_any(_frame, _code, _format, ...) \
  server_error_set1(_frame, SEA_ANY, (u16)_code, _format, ##__VA_ARGS__)

#define server_error_sys(_frame, _code, _format, ...) \
  server_error_set1(_frame, SEA_SYS, (u16)_code, _format, ##__VA_ARGS__)

#endif//__X_SERVER_API_H__