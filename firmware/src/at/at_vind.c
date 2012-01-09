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

#define AT_VSTR 	"VIND"

#include <platform.h>
#include <stdlib.h>

#include "at_v.h"
#include "errno.h"
#include "ve_trace.h"
#include "str_utils.h"
#include "dev_reg_app.h"

/**
 * @addtogroup atvDoc
 * @subsection VIND AT+VIND
 * @par Description:
 * 	Controls error, warning and event reporting.
 *  Every module contains level 31 till 0. \n
 *  The port where the traces are send to is a setting, see @ref VREG\n
 *  The last indications can be stored in memory aswell, see @ref VLOGS
 * @par Act:
 * 	Display indication enabled flags for all modules.
 *
 * @par Parameters:
 * 	<tt>AT+VIND=command,[module],[from],[till]</tt>\n\n
 * 	\c command
 * 		- 0 - disable
 * 		- 1	- enable
 * 		- 2 - save
 * 		.
 * 	\c module
 * 		- none, all modules
 * 		- string e.g. "BMV"
 * 		.
 *	\c from
 *		- (first) level to alter
 *		.
 *	\c till
 *		- not specified, only alter \c from
 *		- if specified, change range \c from / \c till
 *
 * @note
 * 	Errors and warnings are indication level 0 and 1 respectively.
 * @note
 * 	Do not enable too many indication since it can overflow the serial port.
 *
 * @par Examples:
 * 		<tt>AT+VIND=0</tt>				disable all indications\n
 * 		<tt>AT+VIND=1</tt>				enable all indications till level 15\n
 * 		<tt>AT+VIND=0,"RSSI",16</tt>	disable signal strength\n
 * 		<tt>AT+VIND=1,"ALL",0,1</tt>	enable all errors and warnings\n
 * 		<tt>AT+VIND=1,"BMV",0,31</tt>	enable all BMV indications\n
 */
void at_vIndHandler(adl_atCmdPreParser_t* paras)
{
	int command;
	char const* module = NULL;
	VeModule moduleId = VE_MOD_ALL;
	VeModule modLoop;
	int from = -1;
	u32 mask;
	int till = -1;
	int count;

	//
	if (paras->Type == ADL_CMD_TYPE_ACT)
	{
		for (modLoop= 0; modLoop < VE_MOD_COUNT; modLoop++)
			at_vInt("\"%s\",\"0x%08X\"", ve_traceModuleName(modLoop), dev_regs.trace[modLoop]);
		at_vOk();
		return;
	}

	// command
	if (paras->NbPara < 1)
	{
		at_vError();
		return;
	}

	errno = 0;
	command = atoi(ADL_GET_PARAM(paras, 0));
	if (errno != 0)
	{
		at_vErrorTxt("\"Command must be numeric\"");
		return;
	}

	// store command, no further arguments
	if (command == 2)
	{
		ve_traceLevelsStore();
		at_vOk();
		return;
	}

	// get the module
	if (paras->NbPara >= 2)
	{
		module = ADL_GET_PARAM(paras, 1);
		moduleId = ve_traceModuleByName(module);
		if (moduleId == VE_MOD_NONE)
		{
			at_vErrorTxt("\"no such module: '%s'\"", module);
			return;
		}
	}

	// from
	if (paras->NbPara >= 3)
	{
		errno = 0;
		from = atoi(ADL_GET_PARAM(paras, 2));
		if (errno != 0 || from < 0 || from > 31 )
		{
			at_vErrorTxt("\"from invalid: '%s'\"", ADL_GET_PARAM(paras, 2));
			return;
		}
	}

	// till
	if (paras->NbPara >= 4)
	{
		errno = 0;
		till = atoi(ADL_GET_PARAM(paras, 3));
		if (errno != 0 || till < 0 || till > 31 )
		{
			at_vErrorTxt("\"till invalid: '%s'\"", ADL_GET_PARAM(paras, 3));
			return;
		}
	}

	// perform the action itself
	if (command == 1 && from == -1)
	{
		from = 0;
		till = 15;
	}
	// no till specified
	if (till == -1)
		till = from;

	if (till < from)
	{
		at_vErrorTxt("\"Till can't be smaller then from\"");
		return;
	}

	// create a mask for the selected levels
	count = ((till - from) + 1);
	if (from == -1 || count == 32)
	{
		mask = 0xFFFFFFFF;
	} else {
		mask = (1 << count) - 1;
		mask <<= from;
	}

	// loop through all, or the specified module
	modLoop = ( moduleId == VE_MOD_ALL ? 0 : moduleId);
	while (veTrue)
	{
		if (command == 0)
			ve_traceDisableLevels(modLoop, mask);
		else
			ve_traceEnableLevels(modLoop, mask);
		// next module if any
		modLoop++;
		if (moduleId != VE_MOD_ALL || modLoop >= VE_MOD_COUNT)
			break;
	}

	at_vOk();
}

void at_vIndInit(void)
{
	ve_atCmdSubscribe(AT_VCMD, at_vIndHandler, ADL_CMD_TYPE_ACT | ADL_CMD_TYPE_PARA | 0x41);
}
