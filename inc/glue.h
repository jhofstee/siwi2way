#ifndef _GLUE_H_
#define _GLUE_H_

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

#include <stdio.h>

#define adl_InitGetType()	ADL_INIT_POWER_ON
#define adl_memRelease(a)	ve_free(a)

typedef enum {
	ADL_INIT_POWER_ON = -127,
	ADL_INIT_REBOOT_FROM_EXCEPTION,
	ADL_INIT_DOWNLOAD_SUCCESS,
	ADL_INIT_DOWNLOAD_ERROR,
	ADL_INIT_RTC,
	ADL_RET_ERR_SERVICE_LOCKED,
	ERROR,
	OK
} wip_error_t;

typedef void* adl_adInfo_t;
typedef void* wm_lst_t;

/* String */
typedef enum {
	ADL_STR_OK,
	ADL_STR_ERROR,
	ADL_STR_NO_STRING,
} adl_strID_e;

char *adl_strGetResponse(adl_strID_e RspID);

/* port */
#define ADL_GET_PARAM(a,b)		(((b) < (a)->NbPara) ? (a)->params[(b)] : NULL)
#define ADL_AT_PORT_TYPE(a,b)	(((a) << 8) | (b))

typedef enum {
	ADL_PORT_NONE,
	ADL_PORT_UART1,
	ADL_PORT_UART2,
	ADL_PORT_OPEN_AT_VIRTUAL_BASE
} adl_atPort_e;

typedef adl_atPort_e adl_port_e;
typedef enum {
	ADL_AT_UNS,
	ADL_AT_INT,
	ADL_AT_RSP
} adl_rsp_e;

typedef enum {
	ADL_CMD_TYPE_PARA = 0x0100,
	ADL_CMD_TYPE_ACT  = 0x0200,
	ADL_CMD_TYPE_TEST = 0x0400,
	ADL_CMD_TYPE_READ = 0x0800,
} adl_cmd_type;

typedef struct {
	u8 Port;
	u16 Type;
	u8 NbPara;
	u8 NI;
	size_t StrLength;
	void* Contxt;
	char *ParaList;
	char const* params[10];
	char StrData[1];
} adl_atCmdPreParser_t;

typedef struct {
	u8 Dest;
	veBool IsTerminal;
	int StrLength;
	void* Contxt;
	u16 RspID;
	char StrData[1];
} adl_atResponse_t;

typedef void (*adl_atCmdHandler_t)(void*);
typedef void (*adl_atRspHandler_t)(adl_atResponse_t*);

__inline s16 adl_atCmdSubscribe(char *cmdStr, adl_atCmdHandler_t Cmdhdl, u16 Cmdopt) { return OK; }
__inline s16 adl_atCmdUnSubscribe(char *cmdstr, adl_atCmdHandler_t Cmdhdl) { return ERROR; }
__inline s8 adl_atCmdSendExt(char *atstr, adl_atPort_e port, u16 NI, void *ctx, adl_atRspHandler_t rsphdl, ...) { return ERROR; }
__inline s8 adl_atSendResponsePort(int k, adl_atPort_e port, char const* str) {	return ERROR; }
__inline s8 adl_atSendResponse(u16 type, char const *str) {	printf("%s", str); return OK; }
__inline s8 adl_atSendStdResponse(u16 Type, adl_strID_e rspId) { return ERROR; }

/* Flash */
s8 adl_flhSubscribe(char *handle, u16 nbObjectsRes);
s32 adl_flhExist(char *handle, u16 id);
s8 adl_flhRead(char *handle, u16 id, u16 Len, u8 *readData);
s8 adl_flhWrite(char *handle, u16 id, u16 len, u8 *writeData);
s8 adl_flhErase(char *handle, u16 id);

/* Timer */
#define ADL_TMR_TYPE_100MS		0
#define ADL_TMR_TYPE_TICK		1
#define ADL_TMR_S_TO_TICK(a)	a
typedef void* adl_tmr_t;
adl_tmr_t adl_tmrSubscribe(u8, int count, u8 tp, void(*cb)(u8,void*));
s8 adl_tmrUnSubscribe(adl_tmr_t tmr, void(*cb)(u8,void*), u8 tp);

/* RTC */
typedef struct
{
	int Year;
	int Month;
	int Day;
	int Hour;
	int Minute;
	int Second;
} adl_rtcTime_t;
typedef int adl_rtcTimeStamp_t;

typedef enum {
	ADL_RTC_CONVERT_TO_TIMESTAMP
} rtcConvert;

#define ADL_RTC_GET_TIMESTAMP_DAYS(a)			0
#define ADL_RTC_GET_TIMESTAMP_HOURS(a)			0
#define ADL_RTC_GET_TIMESTAMP_MINUTES(a)		0
#define ADL_RTC_GET_TIMESTAMP_SECONDS(a)		0

void adl_rtcGetTime(adl_rtcTime_t* tm);
void adl_rtcConvertTime(adl_rtcTime_t* a, adl_rtcTimeStamp_t* b, rtcConvert conv);
void adl_rtcDiffTime(adl_rtcTimeStamp_t* a, adl_rtcTimeStamp_t* b, adl_rtcTimeStamp_t* c);

#define WIP_SMTP_AUTH_NONE		0
#define WIP_HTTP_VERSION_1_1	11
#define WIP_CHANNEL_INVALID		NULL

typedef enum {
	WIP_CEV_OPEN,
	WIP_CEV_WRITE,
	WIP_CEV_READ,
	WIP_CEV_PEER_CLOSE,
	WIP_CEV_ERROR
} kint_t;

typedef enum wip_cstate_t {
	WIP_CSTATE_BUSY,
	WIP_CSTATE_READY,
	WIP_CSTATE_TO_CLOSE,
	WIP_CSTATE_LAST = WIP_CSTATE_TO_CLOSE
} wip_cstate_t;

typedef u32 wip_in_addr_t;
typedef u32 wip_ethAddr_t;
typedef void* wip_channel_t;
typedef int wip_smtpClientAuthTypes_e;

typedef struct {
	int kind;
	wip_channel_t channel;
	struct content {
		struct error {
			int errnum;
		};
	};
} wip_event_t;

typedef void (*wip_eventHandler_f)(wip_event_t *ev, void *ctx);

s8 wip_netInit(void);
#define wip_debugSetPort(a)
#define wip_debugSetTraceLevel(a)

int wip_close(wip_channel_t c);
int wip_read(wip_channel_t c, void *buffer, u32 buf_len);
int wip_write(wip_channel_t c, void *buffer, u32 buf_len);
int wip_abort(wip_channel_t c);
int wip_shutdown(wip_channel_t c, veBool read, veBool write);
void wip_setCtx(wip_channel_t c, void *ctx);

wip_channel_t wip_TCPClientCreate(const char *serverAddr, u16 serverPort,
								wip_eventHandler_f evHandler, void *ctx);

veBool wip_inet_aton(const char *str, wip_in_addr_t *addr);
veBool wip_inet_ntoa(wip_in_addr_t addr, char *buf, u16 buflen);

int wip_update(int ms);
void glue_main(void);

#endif
