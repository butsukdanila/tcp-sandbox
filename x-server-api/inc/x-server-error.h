#ifndef __X_SERVER_ERROR_H__
#define __X_SERVER_ERROR_H__

#include "x-server-frame.h"

typedef struct {
  u16  auth;       // error source
  u16  code;       // error code
  char text[0xFC]; // error message
} server_error_t;

typedef enum {
  SERR_AUTH_ANY = 0x00, // server error
  SERR_AUTH_SYS = 0x01, // system errno
} server_error_auth_t;

typedef enum {
  SERR_ANY = 0x00, // common server error
  SERR_NOP = 0x01, // request of unknown operation
  SERR_REQ = 0x02, // request client side failure
} server_error_code_t;

void
server_error_set0(server_frame_t *frame, const server_error_t *err);

void
server_error_set1(server_frame_t *frame, u16 auth, u16 code, const char *format, ...);

#define server_error_any(_frame, _code, _format, ...) \
  server_error_set1(_frame, SERR_AUTH_ANY, (u16)_code, _format, ##__VA_ARGS__)

#define server_error_sys(_frame, _code, _format, ...) \
  server_error_set1(_frame, SERR_AUTH_SYS, (u16)_code, _format, ##__VA_ARGS__)

#endif//__X_SERVER_ERROR_H__