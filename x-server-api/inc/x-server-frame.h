#ifndef __X_SERVER_FRAME_H__
#define __X_SERVER_FRAME_H__

#include "x-types.h"

typedef enum {
  SFT_REQ = 0x00,
  SFT_RSP = 0x01,
} server_frame_type_t;

typedef enum {
  SREQ_OP_PING    = 0x00,
  SREQ_OP_MESSAGE = 0x01,
} server_req_opcode_t;

typedef enum {
  SRSP_ST_SUCCESS = 0x00,
  SRSP_ST_FAILURE = 0x01
} server_rsp_status_t;

struct server_frame_head {
  u08 type  :  1;
  u16 flags : 15;
  union { // type dependent
    struct { // request
      u16 opcode;
    } req;
    struct { // response
      u08 status :  1;
      u16 __rsrv : 15;
    } rsp;
  };
  u32 body_sz;
};
typedef struct server_frame_head server_frame_head_t;

struct server_frame {
  server_frame_head_t  head;
  void                *body;
};
typedef struct server_frame server_frame_t;

void
server_frame_head_zero(server_frame_t *frame);

void
server_frame_body_zero(server_frame_t *frame);

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

#define server_frame_rsp() \
  (server_frame_t) { \
    .head.type = SFT_RSP \
  }

#define server_frame_req(_opcode) \
  (server_frame_t) { \
    .head.type = SFT_REQ, \
    .head.req.opcode = _opcode, \
  }

#endif//__X_SERVER_FRAME_H__