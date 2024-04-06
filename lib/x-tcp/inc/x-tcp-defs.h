#ifndef __X_TCP_DEFS_H__
#define __X_TCP_DEFS_H__

#include "x-types.h"

typedef struct {
  u08 op_type :  1;
  u08 op_flag :  7;
  u08 op_sink;
  u16 op_code;
  u32 body_sz;
} x_tcp_frame_head_t;

typedef struct {
  x_tcp_frame_head_t head;
  void *             body;
} x_tcp_frame_t;

typedef enum {
  X_TCP_OP_PING = 0x00
} x_tcp_op_code_t;

typedef enum {
  X_TCP_OP_TYPE_REQ = 0x00,
  X_TCP_OP_TYPE_RSP = 0x01
} x_tcp_op_type_t;

typedef enum {
  X_TCP_OP_FLAG_SUCCESS = 0x00,
  X_TCP_OP_FLAG_FAILURE = 0x01
} x_tcp_op_flag_t;

typedef struct {
  u16  auth;       // error source
  u16  code;       // error code
  char text[0xFC]; // error message
} x_tcp_error_t;

typedef enum {
  X_TCP_ERROR_AUTH_ANY = 0x00, // server error
  X_TCP_ERROR_AUTH_SYS = 0x01, // system errno-like errors
} x_tcp_error_auth_t;

typedef enum {
  X_TCP_ERROR_ANY = 0x00, // common server error
  X_TCP_ERROR_NOP = 0x01, // request of unknown operation
  X_TCP_ERROR_REQ = 0x02, // request client side failure
} x_tcp_error_code_t;

#endif//__X_TCP_DEFS_H__