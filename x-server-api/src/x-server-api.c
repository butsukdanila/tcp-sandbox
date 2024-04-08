#include "x-server-api.h"
#include "x-logs.h"

#include <sys/fcntl.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void
xs_frame_zero(xs_frame_t * frame) {
  if (!frame) return;
  if (frame->body && frame->head.body_sz) {
    memset(frame->body, 0, frame->head.body_sz);
  }
  memset(frame, 0, sizeof(*frame));
}

void
xs_frame_disp(xs_frame_t * frame) {
  if (!frame) return;
  free(frame->body);
}

void
xs_frame_free(xs_frame_t * frame) {
  if (!frame) return;
  xs_frame_disp(frame);
  free(frame);
}

void *
xs_frame_body_realloc(xs_frame_t * frame, u32 body_sz) {
  return frame->body = realloc(frame->body, frame->head.body_sz = body_sz);
}

void
xs_frame_body_set(xs_frame_t * frame, const void * buf, size_t buf_sz) {
  xs_frame_body_realloc(frame, (u32)buf_sz);
  memcpy(frame->body, buf, frame->head.body_sz);
}

int
xs_frame_send(int fd, const xs_frame_t * frame) {
  if (send(fd, &frame->head, sizeof(frame->head), 0) <= 0) {
    return -1;
  }
  u08 *   body    = frame->body;
  ssize_t body_sz = (ssize_t)frame->head.body_sz;
  ssize_t n       = 0;
  while (body_sz > 0) {
    if ((n = send(fd, body, (size_t)body_sz, 0)) <= 0) {
      return -1;
    }
    body    += n;
    body_sz -= n;
  }
  return 0;
}

int
xs_frame_recv(int fd, xs_frame_t * frame) {
  xs_frame_head_t head = {};
  if (recv(fd, &head, sizeof(head), 0) <= 0) {
    return -1;
  }
  memcpy(&frame->head, &head, sizeof(head));
  if (!frame->head.body_sz) {
    return 0;
  }
  u08 *   body    = frame->body = realloc(frame->body, frame->head.body_sz);
  ssize_t body_sz = 0;
  ssize_t n       = 0;
  memset(body, 0, frame->head.body_sz);
  while (body_sz < frame->head.body_sz) {
    if ((n = recv(fd, body, (size_t)frame->head.body_sz, 0)) <= 0) {
      free(frame->body);
      return -1;
    }
    body    += n;
    body_sz += n;
  }
  return 0;
}

int
xs_frame_xchg(int fd, const xs_frame_t * req, xs_frame_t * rsp) {
  if (xs_frame_send(fd, req) < 0) {
    return -1;
  }
  if (xs_frame_recv(fd, rsp) < 0) {
    return -2;
  }
  if (rsp->head.op_flag & XS_OP_FLAG_FAILURE) {
    return -3;
  }
  return 0;
}

void
xs_error_set0(xs_frame_t * frame, const xs_error_t * err) {
  frame->head.op_flag = XS_OP_FLAG_FAILURE;
  xs_frame_body_set(frame, err, sizeof(*err));
  loge("[xs_error][%u][%u] %s", err->auth, err->code, err->text);
}

void
xs_error_set1(xs_frame_t * frame, u08 auth, u08 code, const char * format, ...) {
  xs_error_t err = {
    .auth = auth,
    .code = code,
  };
  va_list args;
  va_start(args, format);
  vsnprintf(err.text, sizeof(err.text), format, args);
  va_end(args);
  xs_error_set0(frame, &err);
}
