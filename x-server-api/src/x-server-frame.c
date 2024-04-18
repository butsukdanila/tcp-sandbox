#include "x-server-frame.h"
#include "x-logs.h"

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