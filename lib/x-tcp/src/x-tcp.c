#include "x-tcp.h"
#include "x-log.h"

#include <sys/socket.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void x_tcp_frame_zero(x_tcp_frame_t * frame) {
  if (!frame) {
    return;
  }
  if (frame->body && frame->head.body_sz) {
    memset(frame->body, 0, frame->head.body_sz);
  }
  memset(frame, 0, sizeof(*frame));
}

void x_tcp_frame_dispose(x_tcp_frame_t * frame) {
  if (!frame) {
    return;
  }
  free(frame->body);
}

void x_tcp_frame_free(x_tcp_frame_t * frame) {
  if (!frame) {
    return;
  }
  x_tcp_frame_dispose(frame);
  free(frame);
}

void x_tcp_frame_body_set(x_tcp_frame_t * frame, const void * buf, size_t buf_sz) {
  frame->head.body_sz = (u32)buf_sz;
  frame->body = realloc(frame->body, frame->head.body_sz);
  memcpy(frame->body, buf, frame->head.body_sz);
}

int x_tcp_frame_send(int fd, const x_tcp_frame_t * frame) {
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

int x_tcp_frame_recv(int fd, x_tcp_frame_t * frame) {
  x_tcp_frame_head_t head;
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

int x_tcp_frame_xchg(int fd, const x_tcp_frame_t * req, x_tcp_frame_t * rsp) {
  if (x_tcp_frame_send(fd, req) < 0) {
    return -1;
  }
  if (x_tcp_frame_recv(fd, rsp) < 0) {
    return -2;
  }
  if (rsp->head.op_flag & X_TCP_OP_FLAG_FAILURE) {
    return -3;
  }
  return 0;
}

void x_tcp_error_set0(x_tcp_frame_t * frame, const x_tcp_error_t * err) {
  frame->head.op_flag = X_TCP_OP_FLAG_FAILURE;
  x_tcp_frame_body_set(frame, err, sizeof(*err));
  loge("[XERR][%u][%u] %s", err->auth, err->code, err->text);
}

void x_tcp_error_set1(x_tcp_frame_t * frame, u08 auth, u08 code, char * tfmt, ...) {
  x_tcp_error_t err = {
    .auth = auth,
    .code = code,
  };
  va_list args;
  va_start(args, tfmt);
  vsnprintf(err.text, sizeof(err.text), tfmt, args);
  va_end(args);
  x_tcp_error_set0(frame, &err);
}
