#include "x-server-error.h"
#include "x-logs.h"

#include <stdarg.h>
#include <stdio.h>

void
server_error_set0(server_frame_t *frame, const server_error_t *err) {
  frame->head.type = SFT_RSP;
  frame->head.rsp.status = SRSP_ST_FAILURE;
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