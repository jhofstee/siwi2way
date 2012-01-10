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

#define VE_MOD VE_MOD_VHTTPC

#include <platform.h>
#include <limits.h>
#include <stdlib.h>

#include <str_utils.h>
#include <ve_assert.h>
#include <ve_httpc.h>
#include <ve_trace.h>

#define TMR_SHOULD_NOT_OCCUR		(10*60)
#define TMR_RETRY					30

#define STRLEN(a)	(sizeof(a) - 1)

typedef int (*ParserCb)(struct VHttpc* httpc, const char* buf, int length, void* ctx);

typedef struct {
	ParserCb cb;
	void* ctx;
} Parser;

static int vhttpc_send(struct VHttpcRequest* req);
static int vhttpc_is_error(int code);
static void tcp_handler(wip_event_t *ev, void *ctx);
static void vhttpc_timeout(void *ctx);

static int parse_http(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_eat_chars(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_number(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_text_till(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_eat_line(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_header_name(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_header_value(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_content(struct VHttpc* httpc, const char* buf, int length, void* ctx);
static int parse_hex_number(struct VHttpc* httpc, const char* buf, int length, void* ctx);

/* Keep in sync with the states enum */
static Parser parsers[] =
{
	{parse_http,			NULL},		/* PARSE_HTTP */
	{parse_number,			NULL},		/* PARSE_VERSION_MAJOR */
	{parse_eat_chars,		"."},		/* PARSE_VERSION_DOT */
	{parse_number,			NULL},		/* PARSE_VERSION_MINOR */
	{parse_eat_chars,		" "},		/* PARSE_SPACES_1 */
	{parse_number,			NULL},		/* PARSE_STATUS */
	{parse_eat_chars,		" "},		/* PARSE_SPACES_2 */
	{parse_text_till,		"\r\n"},	/* PARSE_REASON */
	{parse_eat_line,		NULL},		/* PARSE_STATUS_LINE_CRLF */

	{parse_header_name,		NULL},		/* PARSE_HEADER_NAME */
	{parse_eat_chars,		": \t"},	/* PARSE_HEADER_COLON */
	{parse_header_value,	NULL},		/* PARSE_HEADER_VALUE */
	{parse_eat_line,		NULL},		/* PARSE_HEADER_CRLF */
	{parse_eat_line,		NULL},		/* PARSE_HEAD_END */

	{parse_hex_number,		NULL},		/* PARSE_CHUNK_LENGTH */
	{parse_eat_line,		NULL},		/* PARSE_CHUNK_EXTENSION */
	{parse_content,			NULL},		/* PARSE_CONTENT */
	{parse_eat_line,		NULL},		/* PARSE_CHUNK_CRLF */
};

static void reset_vars(struct VHttpc* httpc)
{
	httpc->parsePos = 0;
}

static void next_state(struct VHttpc* httpc)
{
	httpc->parseState = (ParseState) (httpc->parseState + 1);
	reset_vars(httpc);
}

static void vhttpc_error(struct VHttpc* httpc, int error)
{
	if (httpc->error != 0)
		ve_qtrace("error already set");
	ve_qtrace("error=%d, state=%d", error, httpc->parseState);
	httpc->error = error;
	if (httpc->socket != WIP_CHANNEL_INVALID)
		wip_shutdown(httpc->socket, veTrue, veTrue);
}

static char const *state_name(VHttpcState state)
{
	switch(state)
	{
	case VHTTPC_IDLE:
		return "idle";
	case VHTTPC_RETRY_SOCKET_OPEN:
		return "connect delay";
	case VHTTPC_SOCKET_OPEN:
		return "opening socket";
	case VHTTPC_SENDING_REQUEST:
		return "sending request";
	case VHTTPC_PARSING_REPLY:
		return "reading reply";
	case VHTTPC_ERROR:
		return "error";
	default:
		return "unknown";
	}
}

static void set_state(struct VHttpc* httpc, VHttpcState state, u32 timeout)
{
	ve_qtrace("state: %s %ds", state_name(state), timeout);
	httpc->state = state;
	ve_timer(&httpc->tmr, timeout, vhttpc_timeout, httpc);
}

static int parse_http(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	const char http[] = "HTTP/";
	int n = 0;
	char c;

	while (n < length) {
		c = buf[n++];

		if (c != http[httpc->parsePos++])
			return RET_RSP_MALFORMED;

		if (httpc->parsePos == STRLEN(http)) {
			reset_vars(httpc);
			httpc->parseState = PARSE_VERSION_MAJOR;
			return n;
		}
	}
	return n;
}

// ctx, the chars to eat
static int parse_eat_chars(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int n = 0;

	while (n < length) {
		if (strchr((char*) ctx, buf[n])) {
			httpc->parsePos++;
		} else if (httpc->parsePos == 0) {
			return RET_RSP_MALFORMED;
		} else {
			next_state(httpc);
			return n;
		}
		n++;
	}
	return n;
}

static int parse_eat_line(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	char c;
	int n = 0;

	while (n < length) {
		c = buf[n++];
		if (c == '\n')
		{
			switch (httpc->parseState)
			{
			case PARSE_CHUNK_CRLF:
				if (httpc->parseBuf[0] == '0' && httpc->parseBuf[1] == 0) {
					ve_qtrace("chunk end of response");
					httpc->parseState = PARSE_DONE;
					return n;
				} else {
					httpc->parseState = PARSE_CHUNK_LENGTH;
					return n;
				}
				break;

			case PARSE_HEAD_END:
				httpc->parseState = (httpc->isChunked ? PARSE_CHUNK_LENGTH : PARSE_CONTENT);
				ve_qtrace("header end");
				break;

			case PARSE_STATUS_LINE_CRLF:
			case PARSE_HEADER_CRLF:
				httpc->parseState = PARSE_HEADER_NAME;
				break;

			default:
				next_state(httpc);
				break;
			}
			return n;
		}
	}
	return n;
}

static int parse_number(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int n = 0;
	unsigned long val;
	char *p;

	while (n < length) {
		if (isdigit(buf[n])) {
			if (httpc->parsePos < STRLEN(httpc->parseBuf))
				httpc->parseBuf[httpc->parsePos++] = buf[n];
		} else if (httpc->parsePos == 0) {
			return RET_RSP_MALFORMED;
		} else {
			httpc->parseBuf[httpc->parsePos] = 0;
			val = strtoul(httpc->parseBuf, &p, 0);
			if (*p || val < 0 || val == ULONG_MAX)
				return RET_RSP_MALFORMED;

			switch(httpc->parseState) {
			case PARSE_VERSION_MAJOR:
				httpc->versionMajor = val;
				break;
			case PARSE_VERSION_MINOR:
				httpc->versionMinor = val;
				break;
			case PARSE_STATUS:
				httpc->status = val;
				break;
			default:
				;
			}

			next_state(httpc);
			return n;
		}
		n++;
	}
	return length;
}

/* leaves the delimiter itself untouched */
static int parse_text_till(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int n = 0;

	while (n < length)
	{
		if (strchr((char*) ctx, buf[n])) {
			httpc->parseBuf[httpc->parsePos] = 0;
			next_state(httpc);
			return n;
		} else if (buf[n] == '\r' || buf[n] == '\n') {
			return RET_RSP_MALFORMED;
		} else {
			if (httpc->parsePos < STRLEN(httpc->parseBuf)) {
				httpc->parseBuf[httpc->parsePos] = buf[n];
				httpc->parsePos++;
			} else {
				httpc->parseBuf[0] = ' ';
				httpc->parseBuf[1] = 0;
			}
		}
		n++;
	}
	return n;
}

static int parse_header_name(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int ret;

	/* unfolding, not tested */
	if (httpc->parsePos == 0 && length && (buf[0] == ' ' || buf[0] == '\t')) {
		httpc->parseState = PARSE_HEADER_VALUE;
		return 1;
	}

	ret = parse_text_till(httpc, buf, length, ":");
	if (ret == RET_RSP_MALFORMED) {
		httpc->parseState = PARSE_HEAD_END;
		return 0;
	}

	return ret;
}

static int parse_header_value(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int n = 0;

	while (n < length) {
		if (strchr("\r\n", buf[n])) {
			httpc->headerValue[httpc->parsePos] = 0;

			ve_ltrace(17, "%s = %s", httpc->parseBuf, httpc->headerValue);

			if (stricmp(httpc->parseBuf, "content-length") == 0) {
				char *p;
				httpc->contentLength = strtol(httpc->headerValue, &p, 0);
				if (*p || httpc->contentLength < 0 || httpc->contentLength == INT_MAX)
					return RET_RSP_MALFORMED;
			} else if (stricmp(httpc->parseBuf, "transfer-encoding") == 0) {
				httpc->isChunked = stricmp(httpc->headerValue, "chunked") == 0;
			}

			next_state(httpc);
			return n;
		} else {
			if (httpc->parsePos < STRLEN(httpc->headerValue)) {
				httpc->headerValue[httpc->parsePos] = buf[n];
				httpc->parsePos++;
			} else {
				httpc->headerValue[0] = 0;
			}
		}
		n++;
	}
	return n;
}

static int parse_hex_number(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	int n = 0;

	while (n < length)
	{
		char c = buf[n];

		if (isxdigit(c))
		{
			if (httpc->parsePos >= 8)
				return RET_CHUNK_NO_SPACE;
			httpc->parseBuf[httpc->parsePos++] = toupper(c);
		} else {
			long val;
			char *p;
			httpc->parseBuf[httpc->parsePos] = 0;
			val = strtol(httpc->parseBuf, &p, 16);
			if (*p || p < 0)
				return RET_RSP_MALFORMED;

			if (httpc->parseState == PARSE_CHUNK_LENGTH)
				httpc->contentLength = val;

			next_state(httpc);
			return n;
		}
		n++;
	}
	return n;
}

static int parse_content(struct VHttpc* httpc, const char* buf, int length, void* ctx)
{
	struct VHttpcRequest* req = httpc->reqQueue;
	int n;

	if (httpc->contentLength < 0)
		n = length;
	else
		n = MIN(httpc->contentLength, length);

	if (req->callback) {
		int ret = req->callback(req, REQ_DATA, buf, n);
		if (vhttpc_is_error(ret)) {
			vhttpc_error(httpc, ret);
			return ret;
		}
	}

	if (httpc->contentLength >= 0) {
		httpc->contentLength -= n;
		if (httpc->contentLength == 0)
			httpc->parseState = (httpc->isChunked ? PARSE_CHUNK_CRLF : PARSE_DONE);
	}
	return n;
}

static int parse(struct VHttpc* httpc, char* buf, int length)
{
	char *ptr = buf;

	while(length > 0) {
		int nread;

		if (!httpc->reqQueue)
			return RET_RSP_TOO_LONG;

		nread = parsers[httpc->parseState].cb(httpc, ptr, length, parsers[httpc->parseState].ctx);
		if (nread < 0)
			return nread;
		ve_ltracen(16, "[", ptr, nread);
		length -= nread;
		ptr += nread;

		if (httpc->parseState == PARSE_DONE) {
			int ret;
			/* note: dequeued before the callback so it can be added again */
			struct VHttpcRequest* req = httpc->reqQueue;
			httpc->reqQueue = httpc->reqQueue->next;
			req->next = NULL;
			ret = req->callback(req, REQ_DONE, NULL, 0);
			if (vhttpc_is_error(ret)) {
				vhttpc_error(httpc, ret);
				return ret;
			}
		}
	}
	return RET_OK;
}

static void handle_rx(struct VHttpc* httpc)
{
	char buf[1024];
	int n;
	int ret;

	ve_qtrace("handle_rx");
	do {
		dmemset(buf, 0, sizeof(buf));

		/* read all, also on parse error */
		n = wip_read(httpc->socket, buf, sizeof(buf));
		if (n < 0) {
			ve_qtrace("read error %i", n);
			return;
		}

		ve_ltracen(15, "<", buf, n);

		/* only continue parsing if no errors are encountered */
		if (httpc->error == 0) {
			ret = parse(httpc, buf, n);
			if (vhttpc_is_error(ret))
				vhttpc_error(httpc, ret);
		}
	} while (n);
}

/* Push as much data as possible to the tcp stack */
static int try_to_send(struct VHttpcRequest* req)
{
	struct VHttpc* httpc = req->httpc;
	int n;

	ve_qtrace("sending %p %d", httpc->tx_ptr, httpc->tx_bytes);
	n = wip_write(httpc->socket, httpc->tx_ptr, httpc->tx_bytes);
	if (n < 0) {
		ve_qtrace("Could not write");
		return RET_WRITE_ERROR;
	}

	ve_ltracen(15, ">", httpc->tx_ptr, n);

	httpc->tx_ptr += n;
	httpc->tx_bytes -= n;

	if (httpc->tx_bytes <= 0) {
		ve_qtrace("all data is written..");
		return RET_DONE;
	}

	return RET_OK;
}

static int vhttpc_send_ev(struct VHttpcRequest* req, ReqEvent ev)
{
	struct VHttpc* httpc = req->httpc;

	httpc->tx_bytes = strlen(req->data.data);
	httpc->tx_ptr = req->data.data;
	httpc->parseState = PARSE_HTTP;
	httpc->isChunked = veFalse;
	httpc->parsePos = 0;
	httpc->contentLength = -1;

	if (req->callback) {
		int ret = req->callback(req, ev, NULL, 0);
		if (vhttpc_is_error(ret)) {
			vhttpc_error(httpc, ret);
			return ret;
		}
	}

	if (httpc->socket == WIP_CHANNEL_INVALID) {
		httpc->socket = wip_TCPClientCreate(httpc->host, httpc->port, tcp_handler, httpc);
		if (httpc->socket == WIP_CHANNEL_INVALID) {
			set_state(httpc, VHTTPC_RETRY_SOCKET_OPEN, TMR_RETRY);
			return RET_NO_MEM;
		}

		/* expect WIP_OPEN or WIP_ERROR */
		set_state(httpc, VHTTPC_SOCKET_OPEN, TMR_SHOULD_NOT_OCCUR);
		return RET_OK;
	}

	return try_to_send(req);
}

static int vhttpc_send(struct VHttpcRequest* req)
{
	return vhttpc_send_ev(req, REQ_BEING_SEND);
}

static int vhttpc_resend(struct VHttpcRequest* req)
{
	return vhttpc_send_ev(req, REQ_BEING_SEND_AGAIN);
}

static int vhttpc_enqueue(struct VHttpcRequest* req)
{
	struct VHttpc* httpc = req->httpc;
	int ret;

	req->next = NULL;

	/* if already busy enqueue */
	if (httpc->reqQueue != NULL) {
		struct VHttpcRequest* tail = httpc->reqQueue;
		while (tail->next)
			tail = tail->next;
		tail->next = req;
		return RET_OK;
	}

	httpc->reqQueue = req;
	if (httpc->state == VHTTPC_IDLE) {
		ret = vhttpc_send(req);
		if (ret == RET_DONE)
			set_state(httpc, VHTTPC_PARSING_REPLY, req->read_timeout);
	}

	return RET_OK;
}

int vhttpc_add(struct VHttpcRequest* req, vhttpc_req_callback callback)
{
	req->callback = callback;

	/* For completeness, this would already have triggered a reset though */
	if (req->data.error)
		return RET_NO_MEM;

	return vhttpc_enqueue(req);
}

void vhttpc_req_host(struct VHttpcRequest* req)
{
	str_addf(&req->data, "Host: %s\r\n", req->httpc->host);
}

void vhttpc_req_set(struct VHttpcRequest* req, const char* request_line)
{
	str_set(&req->data, request_line);
	str_add(&req->data, "\r\n");
	vhttpc_req_host(req);
}

void vhttpc_req_add(struct VHttpcRequest* req, const char* header)
{
	str_add(&req->data, header);
	str_add(&req->data, "\r\n");
}

void vhttpc_req_keepalive_timeout(struct VHttpcRequest* req, s32 sec, s32 margin)
{
	req->read_timeout = sec + margin;
	str_addf(&req->data, "Keep-Alive: timeout=%d\r\n", sec);
}

void vhttpc_req_retry(struct VHttpcRequest* req, u32 sec)
{
	ve_assert(req == req->httpc->reqQueue);
	ve_assert(req->httpc->state == VHTTPC_ERROR);

	req->httpc->error = 0;
	set_state(req->httpc, VHTTPC_RETRY_SOCKET_OPEN, sec);
}

/* @note Only call once (or after free) */
void vhttpc_req_init(struct VHttpc* httpc, struct VHttpcRequest* req, u16 length, u16 step)
{
	req->httpc = httpc;
	req->callback = NULL;
	req->read_timeout = TMR_SHOULD_NOT_OCCUR;
	str_new(&req->data, length, step);
}

/* @note Only call this on non queued request. Typically from REQ_DONE */
void vhttpc_req_deinit(struct VHttpcRequest* req)
{
	str_free(&req->data);
}

void vhttpc_init(struct VHttpc* httpc, char const* host, u16 port)
{
	httpc->host = host;
	httpc->port = port;
	httpc->reqQueue = NULL;
	httpc->socket = WIP_CHANNEL_INVALID;
	set_state(httpc, VHTTPC_IDLE, TMR_SHOULD_NOT_OCCUR);
}

void vhttpc_deinit(struct VHttpc* httpc)
{
	/* do not dispose an active client */
	ve_assert(httpc->state == VHTTPC_IDLE && !httpc->reqQueue);
}

static void handle_error(struct VHttpc* httpc, ReqEvent ev)
{
	set_state(httpc, VHTTPC_ERROR, 0);
	if (httpc->reqQueue && httpc->reqQueue->callback)
		httpc->reqQueue->callback(httpc->reqQueue, ev, NULL, 0);
	ve_assert(httpc->state != VHTTPC_ERROR);
}

static void tcp_handler(wip_event_t *ev, void *ctx)
{
	struct VHttpc* httpc = (struct VHttpc*) ctx;
	int ret;

	switch(ev->kind)
	{
	case WIP_CEV_OPEN:
		ve_assert(httpc->state == VHTTPC_SOCKET_OPEN);
		set_state(httpc, VHTTPC_SENDING_REQUEST, TMR_SHOULD_NOT_OCCUR);
		break;

	case WIP_CEV_WRITE:
		if (httpc->error == 0 && httpc->state == VHTTPC_SENDING_REQUEST) {
			ret = try_to_send(httpc->reqQueue);
			if (ret == RET_DONE)
				set_state(httpc, VHTTPC_PARSING_REPLY, httpc->reqQueue->read_timeout);
		}
		break;

	case WIP_CEV_READ:
		ve_assert(httpc->state == VHTTPC_PARSING_REPLY);
		handle_rx(httpc);
		if (httpc->parseState == PARSE_DONE) {
			if (httpc->reqQueue) {
				set_state(httpc, VHTTPC_SENDING_REQUEST, TMR_SHOULD_NOT_OCCUR);
				ret = vhttpc_send(httpc->reqQueue);
				if (ret == RET_DONE)
					set_state(httpc, VHTTPC_PARSING_REPLY, httpc->reqQueue->read_timeout);
			} else {
				set_state(httpc, VHTTPC_IDLE, 0);
			}
		}
		break;

	case WIP_CEV_ERROR:
		ve_qtrace("tcp error");
		wip_close(httpc->socket);
		httpc->socket = WIP_CHANNEL_INVALID;
		handle_error(httpc, REQ_TCP_ERROR);
		break;

	case WIP_CEV_PEER_CLOSE:
		ve_qtrace("peer close");
		wip_close(httpc->socket);
		httpc->socket = WIP_CHANNEL_INVALID;
		if (httpc->state != VHTTPC_IDLE)
			handle_error(httpc, REQ_TCP_PEER_CLOSE);
		break;

	default:
		break;
	}
}

static void vhttpc_timeout(void *ctx)
{
	struct VHttpc* httpc = (struct VHttpc*) ctx;
	switch (httpc->state)
	{
	case VHTTPC_SOCKET_OPEN:
		ve_assert(veFalse);
		break;
	case VHTTPC_RETRY_SOCKET_OPEN:
		ve_assert(httpc->reqQueue != NULL);
		vhttpc_resend(httpc->reqQueue);
		break;
	default:
		ve_warning("timeout occured while '%s'", state_name(httpc->state));
		vhttpc_error(httpc, RET_TIMEOUT);
		break;
	}
}

int vhttpc_is_error(int code)
{
	return code < RET_DONE;
}
