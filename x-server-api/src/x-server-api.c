#include "x-server-api.h"
#include "x-logs.h"

#include <sys/fcntl.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void
server_frame_zero(server_frame_t *frame) {
	if (!frame) return;
	if (frame->body && frame->head.body_sz) {
    memset(frame->body, 0, frame->head.body_sz);
  }
	memset(frame, 0, sizeof(*frame));
}

void
server_frame_disp(server_frame_t *frame) {
  if (!frame) return;
  free(frame->body);
}

void
server_frame_free(server_frame_t *frame) {
  if (!frame) return;
  server_frame_disp(frame);
  free(frame);
}

void *
server_frame_body_realloc(server_frame_t *frame, u32 body_sz) {
  return frame->body = realloc(frame->body, frame->head.body_sz = body_sz);
}

void
server_frame_body_set(server_frame_t *frame, const void *buf, size_t buf_sz) {
  server_frame_body_realloc(frame, (u32)buf_sz);
  memcpy(frame->body, buf, frame->head.body_sz);
}

// int
// server_frame_send(int fd, const server_frame_t *frame) {
//   if (send(fd, &frame->head, sizeof(frame->head), 0) <= 0) {
//     return -1;
//   }
//   u08 *  body    = frame->body;
//   ssize_t body_sz = (ssize_t)frame->head.body_sz;
//   ssize_t n       = 0;
//   while (body_sz > 0) {
//     if ((n = send(fd, body, (size_t)body_sz, 0)) <= 0) {
//       return -1;
//     }
//     body    += n;
//     body_sz -= n;
//   }
//   return 0;
// }

// int
// server_frame_recv(int fd, server_frame_t *frame) {
//   server_frame_head_t head = {};
//   if (recv(fd, &head, sizeof(head), 0) <= 0) {
//     return -1;
//   }
//   memcpy(&frame->head, &head, sizeof(head));
//   if (!frame->head.body_sz) {
//     return 0;
//   }
//   u08 *  body    = frame->body = realloc(frame->body, frame->head.body_sz);
//   ssize_t body_sz = 0;
//   ssize_t n       = 0;
//   memset(body, 0, frame->head.body_sz);
//   while (body_sz < frame->head.body_sz) {
//     if ((n = recv(fd, body, (size_t)frame->head.body_sz, 0)) <= 0) {
//       free(frame->body);
//       return -1;
//     }
//     body    += n;
//     body_sz += n;
//   }
//   return 0;
// }

// int
// server_frame_xchg(int fd, const server_frame_t *req, server_frame_t *rsp) {
//   if (server_frame_send(fd, req) < 0) {
//     return -1;
//   }
//   if (server_frame_recv(fd, rsp) < 0) {
//     return -2;
//   }
//   if (rsp->head.op_flag & SOPF_FAILURE) {
//     return -3;
//   }
//   return 0;
// }

void
server_error_set0(server_frame_t *frame, const server_error_t *err) {
  frame->head.op_flag = SOPF_FAILURE;
  server_frame_body_set(frame, err, sizeof(*err));
	loge("server error [%u][%u]: %s", err->auth, err->code, err->text);
}

void
server_error_set1(server_frame_t *frame, u16 auth, u16 code, const char *format, ...) {
  server_error_t err = {
    .auth = auth,
    .code = code,
  };
  va_list args;
  va_start(args, format);
  vsnprintf(err.text, sizeof(err.text), format, args);
  va_end(args);
  server_error_set0(frame, &err);
}
