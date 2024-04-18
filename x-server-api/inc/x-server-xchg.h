#ifndef __X_SERVER_XCHG_H__
#define __X_SERVER_XCHG_H__

#include "x-server-frame.h"
#include "x-poll-xchg.h"

typedef enum {
  SXG_ST_HEAD = 0,
  SXG_ST_BODY = 1
} server_xchg_state_t;

struct server_xchg {
  pollfd_xchg_t       pfdxg;
  server_xchg_state_t state;
  server_frame_t      frame;
};
typedef struct server_xchg server_xchg_t;

void
server_xchg_recv_prep(server_xchg_t *sxg);

void
server_xchg_send_prep(server_xchg_t *sxg);

void
server_xchg_disp(server_xchg_t *sxg);

int
server_xchg_recv(server_xchg_t *sxg);

int
server_xchg_send(server_xchg_t *sxg);

#endif//__X_SERVER_XCHG_H__