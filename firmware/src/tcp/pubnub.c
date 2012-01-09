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

#define VE_MOD VE_MOD_PUBNUB

#include <platform.h>

#include <pubnub.h>
#include <yajl/yajl_parse.h>
#include <ve_trace.h>

static int json_string(void *ctx, const u8 *buf, size_t bufLen);
static int json_start_array(void *ctx);
static int json_end_array(void *ctx);

static yajl_callbacks callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	json_string,
	NULL,
	NULL,
	NULL,
	json_start_array,
	json_end_array
};

static int json_string(void *ctx, const u8 *buf, size_t bufLen)
{
	struct PubnubRequest* req = (struct PubnubRequest*) ctx;

	if (req->level > 1) {
		if (req->callback)
			req->callback(req, NUB_DATA, (char*) buf, bufLen, req->nub->ctx);
		return 1;
	}

	if (bufLen >= sizeof(req->nub->timeToken))
		return 0;

	memcpy(req->nub->timeToken, buf, bufLen);
	req->nub->timeToken[bufLen] = 0;
	ve_qtrace("token is %s", req->nub->timeToken);
	return 1;
}

static int json_start_array(void * ctx)
{
	struct PubnubRequest* req = (struct PubnubRequest*) ctx;
	req->level++;
	return 1;
}

static int json_end_array(void * ctx)
{
	struct PubnubRequest* req = (struct PubnubRequest*) ctx;
	req->level--;
	return 1;
}

static int json_parse(struct VHttpcRequest* req, ReqEvent ev, char const* buf, int buf_len)
{
	yajl_status stat;
	struct PubnubRequest* nubreq = (struct PubnubRequest*) req->ctx;

	switch(ev)
	{
	case REQ_BEING_SEND_AGAIN:
		ve_qtrace("resend, cleaning up %p", req);
		yajl_free(nubreq->yajl);
		// fall through

	case REQ_BEING_SEND:
		ve_qtrace("sending request %p", req);
		nubreq->yajl = yajl_alloc(&callbacks, NULL, nubreq);
		if (!nubreq->yajl)
			return RET_NO_MEM;
		break;

	case REQ_DATA:
		ve_qtrace("parsing response %p", req);
		if ( (stat = yajl_parse(nubreq->yajl, (u8 const*) buf, buf_len)) != yajl_status_ok ) {
			ve_qtrace("JSON parse error");
			return RET_DATA_PARSE_ERROR;
		}

		if ( (stat = yajl_complete_parse(nubreq->yajl)) != yajl_status_ok ) {
			ve_qtrace("JSON parse error");
			return RET_DATA_PARSE_ERROR;
		}
		break;

	case REQ_TCP_PEER_CLOSE:
		vhttpc_req_retry(req, 1);
		break;

	case REQ_TCP_ERROR:
		vhttpc_req_retry(req, 15);
		break;

	case REQ_DONE:
		ve_qtrace("end of request %p", req);
		yajl_free(nubreq->yajl);
		nubreq->yajl = NULL;
		if (nubreq->callback)
			nubreq->callback(nubreq, NUB_DONE, NULL, 0, nubreq->nub->ctx);
		break;

	default:
		break;
	}

	return RET_OK;
}

int pubnub_init(struct Pubnub* nub, char const* channel, const char* publishKey,
	const char* subscribeKey, const char* secretKey, const char* host, u16 port, void *ctx)
{
	vhttpc_init(&nub->httpc, host, port);
	nub->channel = channel;
	nub->publishKey = publishKey;
	nub->subscribeKey = subscribeKey;
	nub->secretKey = secretKey;
	nub->ctx = ctx;
	strcpy(nub->timeToken, "0");
	return 0;
}

void pubnub_deinit(struct Pubnub* nub)
{
	vhttpc_deinit(&nub->httpc);
}

void pubnub_req_init(struct Pubnub* nub, struct PubnubRequest* nubreq, u16 length, u16 step)
{
	nubreq->level = 0;
	nubreq->nub = nub;
	vhttpc_req_init(&nub->httpc, &nubreq->req, 2000, 200);
}

void pubnub_req_deinit(struct PubnubRequest* nubreq)
{
	vhttpc_req_deinit(&nubreq->req); 	/* free memory associated with the request */
}

static int pubnub_send(struct PubnubRequest* nubreq, pubnub_req_callback callback)
{
	nubreq->req.ctx = nubreq;
	nubreq->callback = callback;
	return vhttpc_add(&nubreq->req, json_parse);
}

// pubsub.pubnub.com/subscribe/sub-key/channel/callback/timetoken
int pubnub_subscribe(struct PubnubRequest* nubreq, const char* timeToken, pubnub_req_callback callback)
{
	Str* s = &nubreq->req.data;
	
	str_set(s, "GET /subscribe/");
	str_addUrlEnc(s, nubreq->nub->subscribeKey);
	str_add(s, "/");
	str_addUrlEnc(s, nubreq->nub->channel);
	str_add(s, "/0/"); // callback
	str_addUrlEnc(s, timeToken);
	str_add(s, " HTTP/1.1\r\n");
	vhttpc_req_host(&nubreq->req); /* host header */
	vhttpc_req_keepalive_timeout(&nubreq->req, 3*60, 30);
	str_add(s, "\r\n");

	return pubnub_send(nubreq, callback);
}

// pubsub.pubnub.com/publish/pub-key/sub-key/signature/channel/callback/message
int pubnub_publish(struct PubnubRequest* nubreq, const char* json, pubnub_req_callback callback)
{
	Str* s = &nubreq->req.data;

	str_set(s, "GET /publish/");
	str_addUrlEnc(s, nubreq->nub->publishKey);
	str_add(s, "/");
	str_addUrlEnc(s, nubreq->nub->subscribeKey);
	str_add(s, "/0/"); // signature
	str_addUrlEnc(s, nubreq->nub->channel);
	str_add(s, "/0/"); // callback
	str_addUrlEnc(s, json);
	str_add(s, " HTTP/1.1\r\n");
	vhttpc_req_host(&nubreq->req);
	str_add(s, "\r\n");

	return pubnub_send(nubreq, callback);
}
