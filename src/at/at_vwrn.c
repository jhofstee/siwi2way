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

#include "platform.h"
#define VE_MOD VE_MOD_ATV
#define AT_VSTR "VWRN"
#include "at_v.h"
#include "ve_trace.h"

/**
 * @addtogroup atvDoc
 * @subsection VWRN AT+VWRN
 * @par Description:
 * 	Controls warnings traces
 * @par Parameters:
 * 	<tt>AT+VWRN=command</tt>\n\n
 *	\c command
 * 		- 0 - disable
 * 		- 1	- enable
 */
static void at_vHandler(adl_atCmdPreParser_t* paras)
{
	switch (at_vGetLong(paras, 0))
	{
	case 0:
		ve_traceDisableWarnings();
		break;

	case 1:
		ve_traceEnableWarnings();
		break;

	default:
		at_vError();
		return;
	}
	at_vOk();
}

void at_vWrnInit(void)
{
	ve_atCmdSubscribe(AT_VCMD, at_vHandler, ADL_CMD_TYPE_PARA | 0x11);
}
