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

#define AT_VSTR "VREG"

#include <at_v.h>
#include <dev_reg_app.h>
#include <str.h>

typedef enum
{
	AT_VREG_VALUES,
	AT_VREG_VALUES_NAME,
	AT_VREG_SET,
	AT_VREG_RESTORE,
	AT_VREG_GROUP
} AtVregCommand;

/**
 * @addtogroup atvDoc
 * @subsection VREG AT+VREG
 * @par Description:
 *  Displays and alters the settings of the device.  After changing values, it
 *  may be necessary to reboot the device before changes will take effect.
 * @par Action syntax:
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG</b>\n
 *     \n
 *     +VREG: 0,\<ID\>,\<Value\>\n
 *     [+VREG: 0,\<ID\>,\<value\>[...]]\n
 *     OK\n
 *     </tt></div>
 * @par Parameters syntax
 *     - If \<Action\>=0 or 1
 *     .
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG=\<Action\>[,\<Starting ID or name\>[,\<Ending ID or name\>]]</b>\n
 *     \n
 *     +VREG: \<Action\>,\<ID\>,\<Value\>[,\<Name\>]\n
 *     [+VREG: \<Action\>,\<ID\>,\<Value\>[,\<Name\>][...]]\n
 *     OK\n
 *     </tt></div>
 *     \n
 *     - If \<Action\>=2
 *     .
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG=\<Mode\>,\<ID or name\>,\<Value\></b>\n
 *     \n
 *     OK\n
 *     </tt></div>
 *     \n
 *     - If \<Action\>=3
 *     .
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG=\<Action\>[,\<Starting ID or name\>[,\<Ending ID or name\>]]</b>\n
 *     \n
 *     OK\n
 *     </tt></div>
 *     \n
 *     - If \<Action\>=4
 *     .
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG=\<Mode\></b>\n
 *     \n
 *     +VREG: 4,\<Group ID\>,\<Group name\>\n
 *     [+VREG: 4,\<Group ID\>,\<Group name\>[...]]\n
 *     </tt></div>
 *     \n
 *     or\n
 *     \n
 *     <div class="atSyntax"><tt>
 *     <b>AT+VREG=\<Mode\>,<Group name or ID\></b>\n
 *     \n
 *     +VREG: 1,\<ID\>,\<Value\>,\<Name\>\n
 *     [+VREG: 1,\<ID\>,\<Value\>,\<Name\>[...]]\n
 *     OK\n
 *     </tt></div>
 * @par Parameters and defined values
 *     @c \<Action\>
 *         - \c 0 - List register IDs and values.
 *         - \c 1 - List register IDs, values, and names.
 *         - \c 2 - Change the value of a register.
 *         - \c 3 - Restore registers to their default values.
 *         - \c 4 - List the available groups, or the registers in a particular
 *         			group.
 *         .
 *         \n
 *         For \<Action\> = 0, 1, or 3, if no registers are specified, the
 *         "common" group is used.  If only a starting register is specified,
 *         only that register will be used.
 */
static void at_vregHandler(adl_atCmdPreParser_t* paras)
{
	Str 					str;
    AtVregCommand			command;
    DevRegId    			fromId = DEV_REG_UNKNOWN;
    DevRegId    			tillId;
    DevRegId 				regId;
    char*					temp;
    DevRegGroup const* 		group;
    veBool					result;

    // Determine the command and register range.
	if(paras->Type == ADL_CMD_TYPE_PARA)
	{
		command = at_vGetLong(paras, 0);
		fromId = dev_regIdFromString(ADL_GET_PARAM(paras, 1));
	}
	else
		command = AT_VREG_VALUES;

	switch (paras->NbPara)
	{
		case 0:
		case 1:
			// Format was either AT+VREG or AT+VREG=<Command>
			fromId = DEV_REG_GRP_COMMON_B;
			tillId = DEV_REG_GRP_COMMON_E;
			break;

		case 2:
			// Format was AT+VREG=<Commmand>,<ID or name>
			tillId = fromId;
			break;

		case 3:
			// Format was either AT+VREG=<Command>,<Start ID or name>,<End ID or name>
			// or AT+VREG=2,<ID or name>,<Value>
			if (command == AT_VREG_SET)
				tillId = fromId;
			else
				tillId = dev_regIdFromString(ADL_GET_PARAM(paras, 2));
			break;

		default:
			at_vError();
			return;
	}

	// Make sure that the register range is valid.
    if (command != AT_VREG_GROUP && (fromId < 0 || fromId >= DEV_REG_COUNT || tillId < 0 || tillId >= DEV_REG_COUNT))
    {
    	at_vError();
    	return;
    }

    // Make sure that the starting register is after the ending register.
    if (tillId < fromId  && command != AT_VREG_SET)
    {
    	DevRegId temp = fromId;
    	fromId = tillId;
    	tillId = temp;
    }

    // Perform the requested action.
    switch(command)
    {
    	// List register IDs and values.
		case AT_VREG_VALUES:
			for (regId = fromId; regId <= tillId; regId++)
			{
				temp = dev_regValue(regId);
				if (temp == NULL)
				{
					at_vError();
					return;
				}

				str_new(&str, 512, 512);
				str_addEscaped(&str, temp);
				at_vInt("0,%d,\"%s\"", regId, str_cstr(&str));
				str_free(&str);

				ve_free(temp);
			}
			break;

		// List register IDs, values and names.
		case AT_VREG_VALUES_NAME:
			for (regId = fromId; regId <= tillId; regId++)
			{
				temp = dev_regValue(regId);
				if (temp == NULL)
				{
					at_vError();
					return;
				}

				str_new(&str, 512, 512);
				str_addEscaped(&str, temp);
				at_vInt("1,%d,\"%s\",\"%s\"", regId, str_cstr(&str), dev_regName(regId));
				str_free(&str);

				ve_free(temp);
			}
			break;

		// Change a register.
		case AT_VREG_SET:
			if (paras->NbPara != 3)
			{
				at_vError();
				return;
			}

			str_new(&str, 512, 512);
			str_addUnescaped(&str, ADL_GET_PARAM(paras, 2));
			if (str.error)
			{
				at_vError();
				return;
			}
			result = dev_regChangeString(fromId, str_cstr(&str));
			str_free(&str);

			if (!result)
			{
				at_vError();
				return;
			}
			break;

		// Reset registers to the default value.
		case AT_VREG_RESTORE:
			for(regId = fromId; regId <= tillId; regId++)
				dev_regRestoreDefault(regId);
			break;

		// List the registers in the specified group.
		case AT_VREG_GROUP:
			if (paras->NbPara == 1)
			{
				// No group specified, so list the available groups.
				for (fromId = 0; fromId < DEV_REG_GROUP_COUNT; fromId++)
				{
					group = dev_regGroupFromId(fromId);
					at_vInt("%d,%d,\"%s\"", command, fromId, group->name);
				}
			}
			else if (paras->NbPara == 2)
			{
				// A group was specified, so list those registers.
				group = dev_regGroupFromString(ADL_GET_PARAM(paras, 1));
				if (group == NULL)
				{
					at_vError();
					return;
				}

				for (regId = group->from; regId <= group->till; regId++)
				{
					if ( (temp = dev_regValue(regId)) == NULL)
					{
						at_vError();
						return;
					}

					str_new(&str, 512, 512);
					str_addEscaped(&str, temp);

					if (str.error)
					{
						at_vError();
						ve_free(temp);
						return;
					}

					at_vInt("1,%d,\"%s\",\"%s\"", regId, str_cstr(&str), dev_regName(regId));
					ve_free(temp);
				}
			}else{
				if (paras->NbPara != 2)
				{
					at_vError();
					return;
				}
			}

			break;

		default:
			// Unknown command
			at_vError();
			return;
    }

    at_vOk();
}

/**
 * Enables the AT+VREG command.
 */
void at_vRegInit(void)
{
	ve_atCmdSubscribe(AT_VCMD, at_vregHandler, ADL_CMD_TYPE_ACT | ADL_CMD_TYPE_PARA | 0x31);
}
