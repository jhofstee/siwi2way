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

#include <stdio.h>

#include <ve_httpc.h>
#include <ve_trace.h>
#include <ve_timer.h>

#if defined(_WIN32)
#pragma comment(lib, "Ws2_32.lib")
#else
typedef int SOCKET;
#endif

typedef struct SocketInfo_s
{
	SOCKET sock;
	wip_eventHandler_f evHandler;
	void* ctx;
	struct SocketInfo_s* next;
	wip_cstate_t state;
} SocketInfo;

static SocketInfo* queue;
static fd_set rx;
static fd_set tx;
static fd_set err;
static fd_set rx_ev;
static fd_set tx_ev;
static fd_set err_ev;

static SocketInfo* find_socket(SocketInfo* q, SOCKET sock, SocketInfo** prev)
{
	SocketInfo* ret = q;
	SocketInfo* last;

	last = NULL;
	while (ret) {
		if (ret->sock == sock) {
			if (prev)
				*prev = last;
			return ret;
		}
		last = ret;
		ret = ret->next;
	}
	return NULL;
}

static void handle_events(fd_set* set, enum wip_event_kind_t kind)
{
	wip_event_t ev;
	unsigned int i;
	SocketInfo* info;
	u_long rxBytes;

	for (i=0; i < set->fd_count; i++)
	{
		SOCKET fd = set->fd_array[i];

		if ((info = find_socket(queue, fd, NULL)) == NULL) {
			ve_qtrace("bug, socket not found");
			continue;
		}

		if (kind == WIP_CEV_READ) {
			if (ioctlsocket(info->sock, FIONREAD, &rxBytes) != 0 || rxBytes == 0 ) {
				kind = WIP_CEV_PEER_CLOSE;
				info->state = WIP_CSTATE_TO_CLOSE;
			}
		}

		if (kind == WIP_CEV_WRITE) {
			if (info->state == WIP_CSTATE_BUSY) {
				info->state = WIP_CSTATE_READY;
				if (queue->evHandler) {
					ev.kind = WIP_CEV_OPEN;
					queue->evHandler(&ev, queue->ctx);
				}
			}
			FD_CLR(fd, &tx);
		}

		if (info->evHandler) {
			ev.kind = kind;
			info->evHandler(&ev, info->ctx);
		}

		switch (kind) {
		case WIP_CEV_ERROR:
		case WIP_CEV_PEER_CLOSE:
			FD_CLR(fd, &rx);
			FD_CLR(fd, &tx);
			FD_CLR(fd, &err);
			FD_CLR(fd, &rx_ev);
			FD_CLR(fd, &tx_ev);
			break;
		default:
			;
		}
	}
}

int wip_update(int ms)
{
	struct timeval waitd;
	int n;

	waitd.tv_sec = 0;
	waitd.tv_usec = ms;
	rx_ev = rx;
	tx_ev = tx;
	err_ev = rx;

	n = select(0, &rx_ev, &tx_ev, &err_ev, &waitd);
	if (n < 0)
		return RET_NOT_IMPLEMENTED;

	if (n == 0)
		return RET_OK;

	handle_events(&err_ev, WIP_CEV_ERROR);
	handle_events(&rx_ev, WIP_CEV_READ);
	handle_events(&tx_ev, WIP_CEV_WRITE);
	return RET_OK;
}


wip_channel_t wip_TCPClientCreate(const char *serverAddr, u16 serverPort,
									wip_eventHandler_f evHandler, void *ctx)
{
	struct addrinfo* ai = NULL;
	struct addrinfo hints;
	char port[6];
	int err;
	u_long nonblock = 1;
	SocketInfo* info = (SocketInfo*) ve_calloc(sizeof(SocketInfo), 1);

	info->ctx = ctx;
	info->evHandler = evHandler;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	sprintf(port, "%d", serverPort);
	if (getaddrinfo(serverAddr, port, &hints, &ai) != 0)
		return NULL;

	/// XXX: FD_SETSIZE
	info->sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (ioctlsocket(info->sock, FIONBIO, &nonblock) != 0)
		goto cleanup;
	/*
	int x = fcntl(s,F_GETFL,0);
	fcntl(s,F_SETFL,x | O_NONBLOCK);
	*/
	err = connect(info->sock, ai->ai_addr, ai->ai_addrlen);
	if (err != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
		goto cleanup;
	freeaddrinfo(ai);

	info->state = WIP_CSTATE_BUSY;
	FD_SET(info->sock, &rx);
	FD_SET(info->sock, &tx);

	if (queue) {
		SocketInfo* tail = queue;
		while (tail->next)
			tail = tail->next;
		tail->next = info;
	} else {
		queue = info;
	}
	return (wip_channel_t) info;

cleanup:
	ve_free(info);
	freeaddrinfo(ai);
	return NULL;
}

void wip_setCtx(wip_channel_t c, void *ctx)
{
	SocketInfo* info = (SocketInfo*) c;
	info->ctx = ctx;
}

int wip_write(wip_channel_t c, void *buffer, u32 buf_len)
{
	SocketInfo* info = (SocketInfo*) c;
	return send(info->sock, (const char*) buffer, buf_len, 0);
}

int wip_read(wip_channel_t c, void *buffer, u32 buf_len)
{
	SocketInfo* info = (SocketInfo*) c;
	int ret = recv(info->sock, (char*) buffer, buf_len, 0);
	if (ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
		return 0;
	return ret;
}

int wip_shutdown(wip_channel_t c, BOOL read, BOOL write)
{
	SocketInfo* info = (SocketInfo*) c;
	int ret = shutdown(info->sock, SD_BOTH);
	return 0;
}

int wip_close(wip_channel_t c)
{
	SocketInfo* info = (SocketInfo*) c;
	SocketInfo* prev;

	if ((info = find_socket(queue, info->sock, &prev)) == NULL)
		return -1;
	if (prev)
		prev->next = queue;
	else
		queue = info->next;
	closesocket(info->sock);
	ve_free(info);
	return 0;
}

s8 wip_netInitOpts(int opt, ...)
{
#if defined(_WIN32)
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != NO_ERROR)
		return -1;
#endif
	return 0;
}

s8 wip_netInit(void)
{
	wip_netInitOpts(0, 0);
	return 0;
}

BOOL wip_inet_aton(const char *str, wip_in_addr_t *addr)
{
	return FALSE;
}

BOOL wip_inet_ntoa(wip_in_addr_t addr, char *buf, u16 buflen)
{
	strcpy(buf, "address");
	return FALSE;
}

char *adl_strGetResponse(adl_strID_e RspID)
{
	switch(RspID)
	{
	case ADL_STR_OK:
		return ve_strdup("OK");
	case ADL_STR_ERROR:
		return ve_strdup("ERROR");
	default:
		return ve_strdup("Unknown string");
	}
}
