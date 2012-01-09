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

#define VE_MOD	VE_MOD_PUBNUBAT

#include <platform.h>
#include <pubnub_at.h>
#include <ve_at.h>
#include <ve_trace.h>

static void publish_callback(struct PubnubRequest* req, NubEv ev,
									char const* buf, int buf_len, void *ctx)
{
	struct PubnubAt* nubat = (struct PubnubAt*) ctx;

	switch (ev)
	{
	case NUB_DONE:
		pubnub_req_deinit(req);			/* destruct the request data */
		ve_free(req);					/* free the request itself */
		pubnub_atSubscribe(nubat);		/* wait for commands when idle */
		break;

	default:
		;
	}
}

static veBool at_rspHandler(adl_atResponse_t *params)
{
	struct PubnubAt* nubat = (struct PubnubAt*) params->Contxt;

	ve_qtrace("rsp '%s' %p %d", params->StrData, nubat, params->IsTerminal);
	pubnub_atPublish(nubat, params->StrData);

	if (params->IsTerminal)
		nubat->atCmdPending = veFalse;

	return veFalse;
}

static void subscribe_callback(struct PubnubRequest* req, NubEv ev,
									char const* buf, int buf_len, void *ctx)
{
	struct PubnubAt* nubat = (struct PubnubAt*) ctx;

	switch (ev)
	{
	case NUB_DATA:
		{
			Str str;
			if (!str_newn(&str, buf, buf_len))
				return;

			/* This relies on the fact that command always sends at terminal response! */
			nubat->atCmdPending = veTrue;
			if (ve_atCmdSendExt(str.data, veFalse, 0, nubat, at_rspHandler) != OK)
				nubat->atCmdPending = veFalse;
			str_free(&str);
			break;
		}

	case NUB_DONE:
		pubnub_atSubscribe(nubat);	/* wait for commands when idle */
		break;

	default:
		;
	}
}

/* wait for commands when idle */
void pubnub_atSubscribe(struct PubnubAt* nubat)
{
	if (nubat->nub.httpc.reqQueue || nubat->atCmdPending)
		return;
	pubnub_subscribe(&nubat->subReq, nubat->nub.timeToken, subscribe_callback);
}

veBool pubnub_atPublishN(struct PubnubAt* nubat, char const *buf, size_t buf_len)
{
	struct PubnubRequest *nubreq;
	u8 const *json;
	size_t json_len;

	/* allocate request */
	if (!nubat->g) {
		nubat->g = yajl_gen_alloc(NULL);
		if (!nubat->g)
			return veFalse;
	}

	nubreq = (struct PubnubRequest *) ve_malloc(sizeof(struct PubnubRequest));
	if (!nubreq) {
		/* reuse the nubat->q next time */
		return veFalse;
	}

	pubnub_req_init(&nubat->nub, nubreq, 512, 512);

	/* build json data.. */
	if (yajl_gen_string(nubat->g, (u8*) buf, buf_len) != yajl_gen_status_ok) {
		ve_error("json: not a valid string");
		goto error;
	}

	if (yajl_gen_get_buf(nubat->g, &json, &json_len) != yajl_gen_status_ok) {
		ve_error("json: could not get buf");
		return veFalse;
	}

	/* sent it */
	if (pubnub_publish(nubreq, (char*) json, publish_callback) != RET_OK) {
		ve_error("could not pubnub_publish");
		goto error;
	}

	yajl_gen_clear(nubat->g);	/* empty buffers */
	yajl_gen_free(nubat->g);
	nubat->g = NULL;
	return veTrue;

error:
	pubnub_req_deinit(nubreq);
	ve_free(nubreq);
	yajl_gen_free(nubat->g);
	nubat->g = NULL;

	return veFalse;
}

veBool pubnub_atPublish(struct PubnubAt* nubat, char const *str)
{
	return pubnub_atPublishN(nubat, str, strlen(str));
}

int pubnub_atInit(struct PubnubAt* nubat, char const* channel, const char* publishKey,
		const char* subscribeKey, const char* secretKey, const char* host, u16 port)
{
	pubnub_init(&nubat->nub, channel, publishKey, subscribeKey, secretKey, host, port, nubat);
	pubnub_req_init(&nubat->nub, &nubat->subReq, 512, 512);
	nubat->atCmdPending = veFalse;
	nubat->g = NULL;

	return RET_OK;
}

void pubnub_atDeinit(struct PubnubAt* nubat)
{
	pubnub_deinit(&nubat->nub);
	pubnub_req_deinit(&nubat->subReq);
	if (nubat->g) {
		yajl_gen_free(nubat->g);
		nubat->g = NULL;
	}
}
