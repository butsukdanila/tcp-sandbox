#ifndef __X_SERVER_API_DEFS_H__
#define __X_SERVER_API_DEFS_H__

#include "x-types.h"

typedef struct {
  u08 op_type :  1;
  u16 op_flag : 15;
  u16 op_code;
  u32 body_sz;
} server_frame_head_t;

typedef struct {
  server_frame_head_t  head;
  void                *body;
} server_frame_t;

typedef enum {
  SOPC_PING    = 0x00,
  SOPC_MESSAGE = 0x01,
} server_op_code_t;

typedef enum {
  SOPT_REQ = 0x00,
  SOPT_RSP = 0x01,
} server_op_type_t;

typedef enum {
  SOPF_SUCCESS = 0x00,
  SOPF_FAILURE = 0x01
} server_op_flag_t;

typedef struct {
  u16  auth;       // error source
  u16  code;       // error code
  char text[0xFC]; // error message
} server_error_t;

typedef enum {
  SEA_ANY = 0x00, // server error
  SEA_SYS = 0x01, // system errno
} server_error_auth_t;

typedef enum {
  SE_ANY = 0x00, // common server error
  SE_NOP = 0x01, // request of unknown operation
  SE_REQ = 0x02, // request client side failure
} server_error_code_t;

#endif//__X_SERVER_API_DEFS_H__