#ifndef _PUBNUD_H_
#define _PUBNUD_H_

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

#include "ve_httpc.h"
#include "yajl/yajl_parse.h"

typedef enum {
	NUB_DATA,
	NUB_ERROR,
	NUB_DONE
} NubEv;

struct PubnubRequest;

typedef void (*pubnub_req_callback)(struct PubnubRequest* req, NubEv ev,
									char const* buf, int buf_len, void *ctx);

struct Pubnub {
	char const* channel;
	char const* publishKey;
	char const* subscribeKey;
	char const* secretKey;
	char timeToken[20];
	struct VHttpc httpc;
	void *ctx;
};

struct PubnubRequest {
	struct Pubnub* nub;
	struct VHttpcRequest req;
	yajl_handle yajl;
	int level;
	pubnub_req_callback callback;
};

int pubnub_init(struct Pubnub* nub, char const* channel, const char* publishKey,
					const char* subscribeKey, const char* secretKey,
					const char* host, u16 port, void *ctx);
void pubnub_deinit(struct Pubnub* nub);

void pubnub_req_init(struct Pubnub* nub, struct PubnubRequest* req, u16 length, u16 step);
void pubnub_req_deinit(struct PubnubRequest* nubreq);
int pubnub_subscribe(struct PubnubRequest* nubreq, const char* timeToken, pubnub_req_callback callback);
int pubnub_publish(struct PubnubRequest* nubreq, const char* json, pubnub_req_callback callback);

#endif
