#ifndef _AT_V_H_
#define _AT_V_H_

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
#include <at_v.def.h>
#include <str.h>
#include <str_utils.h>
#include <ve_at.h>

void at_vInit(void);

/* prototypes like void at_vErrInit(void); */
#define X(_at) void at_ ## _at ## Init(void);
	VAT_CMDS
#undef X

// helpers
long at_vGetLong(adl_atCmdPreParser_t* paras, int n);

void at_vVebusResponse(Str* msg, float** roi, int length, int regId);


#ifdef AT_VSTR

// Standardised macro's for output format, might we ever consider
// removing the standard, but annoying additional \r\n or change the
// name of a AT command.
#define AT_VCMD 	"AT+"AT_VSTR
#define AT_VB(str) 	"\r\n+"AT_VSTR": "str
#define AT_VE(str)	str"\r\n"
#define AT_V(str) 	AT_VB(str) AT_VE("")

#define 	at_vInt(format,...) at_vIntPort(paras->Port,format,##__VA_ARGS__)
#define		at_vIntPort(port,format,...) str_atSendResponse(ADL_AT_PORT_TYPE(port,ADL_AT_INT),AT_V(format),##__VA_ARGS__)
#define 	at_vUns(format,...) str_atSendResponse(ADL_AT_UNS,AT_V(format),##__VA_ARGS__)

// for the requesting port, normal situation
#define		at_vOk() 						str_atOk(paras->Port)
#define		at_vSuccess(ok)					str_atSuccess(paras->Port, ok)
#define 	at_vError() 					str_atError(paras->Port)
#define 	at_vErrorTxt(format, ...) 		str_atErrorTxt(paras->Port,AT_V(format), ##__VA_ARGS__)

#endif

#endif
