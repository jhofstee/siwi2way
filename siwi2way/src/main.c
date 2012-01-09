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

#include <platform.h>

#ifndef __OAT_API_VERSION__
#include <direct.h>
#include <stdlib.h>
#endif

#include <at_v.h>
#include <dev_reg_app.h>
#include <pubnub.h>
#include <pubnub_at.h>
#include <str_utils.h>
#include <ve_at.h>
#include <ve_httpc.h>
#include <ve_timer.h>
#include <ve_trace.h>

extern void CfgEth(void (*)(void));
static void go(void);

static int print_req(struct VHttpcRequest* req, ReqEvent ev, char const* buf, int buf_len)
{
	switch(ev) {
	case REQ_BEING_SEND:
		break;

	case REQ_DATA:
		ve_qtracen("data: '", buf, buf_len);
		break;

	case REQ_TCP_ERROR:
	case REQ_PARSE_ERROR:
		vhttpc_req_retry(req, 60);
		break;

	case REQ_HTTPC_CAN_BE_CLOSED:
		break;

	case REQ_DONE:
		/* Free memory used for the request */
		vhttpc_req_deinit(req);
		break;

	default:
		;
	}

	return RET_OK;
}

void print_nub(struct PubnubRequest* nubreq, NubEv ev, char const* buf,
												int buf_len, void *ctx)
{
	switch(ev)
	{
	case NUB_DATA:
		ve_qtracen("chat:", buf, buf_len);
		break;

	case NUB_ERROR:
		break;

	case NUB_DONE:
		ve_qtrace("nub done");
		pubnub_subscribe(nubreq, nubreq->nub->timeToken, print_nub);
		break;
	}
}

void http_get_example(void)
{
	static struct VHttpc httpc;
	static struct VHttpcRequest req;

	/* init tcp connection */
	vhttpc_init(&httpc, "www.loremipsum.de", 80);

	/*
	 * Tie request to connection, numbers are initial size of request
	 * and the number of bytes the request is expanded with when needed.
	 */
	vhttpc_req_init(&httpc, &req, 512, 512);

	/* The request itself */
	vhttpc_req_set(&req, "GET /downloads/version3.txt HTTP/1.1");
	vhttpc_req_add(&req, "Connection: close");
	vhttpc_req_add(&req, ""); /* end of headers */

	/*
	 * Add to the request queue of the tcp connection. The only valid
	 * reason this could fail should be out of memory.
	 */
	if (vhttpc_add(&req, print_req) != 0)
		ve_qtrace("could not create request");
}

// see www.pubnub.com/blog/build-real-time-web-apps-easy
void pubnub_demo_chat(void)
{
	static struct Pubnub nub;
	static struct PubnubRequest nubreq;

	/* The underlying tcp connection, should reconnect in case of failure.. */
	pubnub_init(&nub, "chat", "demo", "demo", "0", "pubsub.pubnub.com", 80, NULL);
	pubnub_req_init(&nub, &nubreq, 512, 512);
	pubnub_publish(&nubreq, "\"Hello World From c\"", print_nub);
}

void pubnub_account(void)
{
	static struct Pubnub nub;
	static struct PubnubRequest nubreq;

	/* The underlying tcp connection, should reconnect in case of failure.. */
	pubnub_init(&nub, "6356800465306", dev_regs.pubnubPublishKey, dev_regs.pubnubSubscribeKey, 
									"0", "pubsub.pubnub.com", 80, NULL);
	pubnub_req_init(&nub, &nubreq, 512, 512);
	pubnub_publish(&nubreq, "\"Hello World From c\"", print_nub);
}

void pubnub_at_console(void)
{
	static struct PubnubAt nubat;
	
	/* Initializes an idle connection... */
	pubnub_atInit(&nubat, "my_channel", dev_regs.pubnubPublishKey, dev_regs.pubnubSubscribeKey, 
									"0", "pubsub.pubnub.com", 80);

	/* Something must be done to get it started.. */
	if (1)
		pubnub_atPublish(&nubat, "\r\nAT+HELLO_WORLD\r\n"); /* sent */
	else
		pubnub_atSubscribe(&nubat); /* listen*/

	/* From here on it should take care of itself */
}

void init(void)
{
	ve_timer_init();
	dev_regInit();
	at_vInit();			/* commands registered on it */
}

#ifndef __OAT_API_VERSION__

int main(int argc, char const* argv[])
{
	mkdir("regs");
	init();
	wip_netInit();
	go();
	glue_main();
	return 0;
}

#else
void bearer_opened(void)
{
	ve_qtrace("bearer opened...");
	go();
}

void startup(u8 id, void* context)
{
	init();

	ve_qtrace("opening bearer...");
	wip_netInitOpts(WIP_NET_OPT_END);
	wip_debugSetPort(WIP_NET_DEBUG_PORT_UART1);
	CfgEth(bearer_opened);
}

void main_task(void)
{
	adl_tmrSubscribe(veFalse, 100, ADL_TMR_TYPE_100MS, startup);
}

#endif

static void go(void)
{
	//http_get_example();
	//pubnub_demo_chat();
	//pubnub_account();
	pubnub_at_console();
}
