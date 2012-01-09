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

#include <errno.h>
#include <stdlib.h>

#include <at_v.def.h>
#include <at_v.h>


/**
 * @defgroup atvDoc AT Commands
 * @ingroup vgrDoc
 *
 * @section intro Introduction
 * The Victron AT commands are available at an AT processing serial connection and
 * can be executed from other sources aswell, e.g. send by SMS.
 *
 * @par Test:
 * Describes the action corresponding to the AT command + question mark, e.g. AT+VLOGS?
 *
 * @par Parameters:
 * Describes the handling of parameters, e.g. AT+VADRM=1
 */

/** @addtogroup atvDoc
 * @section index Command index
 */

void at_vInit(void)
{
#define X(_at) at_ ## _at ## Init();
	VAT_CMDS
#undef X	
}

/**
 * converts a parameter string to an integer.
 * assuming number is positive. -1 is error.
 */
int at_vGetInt(adl_atCmdPreParser_t* paras, int n)
{
	int command;

	if (paras->Type != ADL_CMD_TYPE_PARA)
		return -1;

	if (n >= paras->NbPara)
		return -1;

	// check arguments
	errno = 0;
	command = atoi(ADL_GET_PARAM(paras, n));
	if (errno != 0)
		return -1;

	return command;
}
