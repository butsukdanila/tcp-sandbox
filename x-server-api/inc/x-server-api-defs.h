#ifndef __X_SERVER_API_DEFS_H__
#define __X_SERVER_API_DEFS_H__

#include "x-types.h"

typedef struct {
  u08 op_type :  1;
  u08 op_flag :  1;
  u16 op_slot : 14;
  u16 op_code;
  u32 body_sz;
} xs_frame_head_t;

typedef struct {
  xs_frame_head_t head;
  void *          body;
} xs_frame_t;

typedef enum {
  XS_OP_PING    = 0x00,
  XS_OP_MESSAGE = 0x01,
} xs_op_code_t;

typedef enum {
  XS_OP_TYPE_REQ = 0x00,
  XS_OP_TYPE_RSP = 0x01
} xs_op_type_t;

typedef enum {
  XS_OP_FLAG_SUCCESS = 0x00,
  XS_OP_FLAG_FAILURE = 0x01
} xs_op_flag_t;

typedef struct {
  u16  auth;       // error source
  u16  code;       // error code
  char text[0xFC]; // error message
} xs_error_t;

typedef enum {
  XS_ERROR_AUTH_ANY = 0x00, // server error
  XS_ERROR_AUTH_SYS = 0x01, // system errno
} xs_error_auth_t;

typedef enum {
  XS_ERROR_ANY = 0x00, // common server error
  XS_ERROR_NOP = 0x01, // request of unknown operation
  XS_ERROR_REQ = 0x02, // request client side failure
} xs_error_code_t;

#endif//__X_SERVER_API_DEFS_H__