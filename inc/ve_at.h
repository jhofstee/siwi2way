#ifndef _VE_AT_H_
#define _VE_AT_H_
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

/// The port used for internal AT commands
#define ADL_PORT_VICTRON	(ADL_PORT_OPEN_AT_VIRTUAL_BASE + 0x10)

// free response
#define ve_atSendResponsePort(_t,_p,_r) ve_atSendResponse(ADL_AT_PORT_TYPE(_p,_t),_r)
s32 	ve_atSendResponse(u16 Type, char const *Text);

// standard response
#define ve_atSendStdResponsePort(_t,_p,_r) ve_atSendStdResponse(ADL_AT_PORT_TYPE(_p,_t),_r)
s32 	ve_atSendStdResponse(u16 Type, adl_strID_e RspID);

s8 		ve_atCmdSendExt(char *atstr, adl_atPort_e port, u16 NI, void* Contxt, adl_atRspHandler_t rsphdl);
s16 	ve_atCmdSubscribe(char const *Cmdstr, adl_atCmdHandler_t Cmdhdl, u16 Cmdopt);
s8 		ve_atCmdCreate(char *atstr, u16 rspflag, adl_atRspHandler_t rsphdl);

// testing
adl_atResponse_t* ve_atAllocAtResponse(char const *str, veBool addCrLf, u16 Type);

#endif
