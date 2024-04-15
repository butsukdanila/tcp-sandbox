#ifndef __X_SERVER_API_XCHG_H__
#define __X_SERVER_API_XCHG_H__

#include "x-server-api.h"
#include "x-poll.h"

struct server_xchg_ctx {
	pollfd_t *pfd;
	void     *buf;
	size_t    trg_sz;
	size_t    cur_sz;
};
typedef struct server_xchg_ctx server_xchg_ctx_t;

int
server_xchg_ctx_send(server_xchg_ctx_t *ctx);

int
server_xchg_ctx_recv(server_xchg_ctx_t *ctx);

typedef enum {
	SXR_RUPTURE = -2,
	SXR_FAILURE = -1,
	SXR_IGNORED =  0,
	SXR_SUCCESS =  1,
	SXR_PARTIAL =  2,
} server_xchg_result_t;

typedef enum {
	SXS_HEAD = 0,
	SXS_BODY = 1
} server_xchg_state_t;

struct server_frame_xchg {
	server_xchg_ctx_t   ctx;
	server_xchg_state_t state;
	server_frame_t      frame;
};
typedef struct server_frame_xchg server_frame_xchg_t;

void
server_frame_xchg_recv_prepare(server_frame_xchg_t *xchg);

void
server_frame_xchg_send_prepare(server_frame_xchg_t *xchg);

void
server_frame_xchg_disp(server_frame_xchg_t *xchg);

int
server_frame_xchg_recv(server_frame_xchg_t *xchg);

int
server_frame_xchg_send(server_frame_xchg_t *xchg);

#endif//__X_SERVER_API_XCHG_H__