#ifndef _VHTTPC_H_
#define _VHTTPC_H_

/*
 * Copyright (c) 2012 All rights reserved.
 * Jeroen Hofstee, Victron Energy, jhofstee@victronenergy.com.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <str.h>
#include <ve_timer.h>

typedef enum {
	/* Errors */
	RET_CHUNK_NO_SPACE = -9999,
	RET_RSP_HEADER_TOO_LONG = -9998,
	RET_WRITE_ERROR = -9997,
	RET_NO_MEM = -9996,
	RET_RSP_MALFORMED = -9995,
	RET_RSP_TOO_LONG = -9994,
	RET_NOT_IMPLEMENTED = -9993,
	RET_TIMEOUT = -9992,

	/* HTTP payload errors */
	RET_DATA_PARSE_ERROR = -9000,
	RET_DONE = -1,
	RET_OK = 0,
} RetCode;

typedef enum {
	REQ_BEING_SEND,
	REQ_BEING_SEND_AGAIN,
	REQ_DATA,
	REQ_DONE,
	REQ_TCP_ERROR,
	REQ_TCP_PEER_CLOSE,
	REQ_PARSE_ERROR,
	REQ_HTTPC_CAN_BE_CLOSED,
} ReqEvent;

/* Keep in sync with the callbacks */
typedef enum {
	PARSE_HTTP,
	PARSE_VERSION_MAJOR,
	PARSE_VERSION_DOT,
	PARSE_VERSION_MINOR,
	PARSE_SPACES_1,
	PARSE_STATUS,
	PARSE_SPACES_2,
	PARSE_REASON,
	PARSE_STATUS_LINE_CRLF,
	PARSE_HEADER_NAME,
	PARSE_HEADER_COLON,
	PARSE_HEADER_VALUE,
	PARSE_HEADER_CRLF,
	PARSE_HEAD_END,

	PARSE_CHUNK_LENGTH,
	PARSE_CHUNK_EXTENSION,
	PARSE_CONTENT,
	PARSE_CHUNK_CRLF,

	PARSE_DONE
} ParseState;

typedef enum {
	VHTTPC_IDLE,
	VHTTPC_RETRY_SOCKET_OPEN,
	VHTTPC_SOCKET_OPEN,
	VHTTPC_SENDING_REQUEST,
	VHTTPC_PARSING_REPLY,
	VHTTPC_ERROR,
} VHttpcState;

struct VHttpcRequest;

struct VHttpc
{
	struct VHttpcRequest* reqQueue;
	wip_channel_t socket;
	char const* host;
	u16 port;

	ParseState parseState;
	int parsePos;
	char parseBuf[100];
	char headerValue[100];
	int error;

	s32 contentLength;
	s32 versionMajor;
	s32 versionMinor;
	s32 status;
	veBool isChunked;

	char* tx_ptr;
	int tx_bytes;
	VHttpcState state;
	struct VeTimer tmr;
};

typedef int (*vhttpc_req_callback)(struct VHttpcRequest* req, ReqEvent ev,
												char const* buf, int buf_len);

struct VHttpcRequest
{
	Str data;
	void* ctx;

	/* "private" */
	s32 read_timeout;
	vhttpc_req_callback callback;
	struct VHttpc* httpc;
	struct VHttpcRequest* next;
};

void vhttpc_init(struct VHttpc* httpc, char const* host, u16 port);
void vhttpc_deinit(struct VHttpc* httpc);

void vhttpc_req_init(struct VHttpc* httpc, struct VHttpcRequest* req, u16 length, u16 step);
void vhttpc_req_set(struct VHttpcRequest* req, const char* request_line);
void vhttpc_req_add(struct VHttpcRequest* req, const char* header);
void vhttpc_req_host(struct VHttpcRequest* req);
void vhttpc_req_keepalive_timeout(struct VHttpcRequest* req, s32 sec, s32 margin);

/* possible actions on error */
void vhttpc_req_retry(struct VHttpcRequest* req, u32 sec);

/* must be called from REG_DONE */
void vhttpc_req_deinit(struct VHttpcRequest* req);

int vhttpc_add(struct VHttpcRequest* req, vhttpc_req_callback callback);

#endif
