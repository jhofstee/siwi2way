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

#define VE_MOD 	VE_MOD_REG
#define VE_DEV_REG_DEFINE_CONSTANTS

#include <dev_reg_app.h>

/**
 * preprocessor magic: build a table with information about the settings
 * of this device contains name, dst, default, type
 */
#define XR(_enm,_str,_var,_def,_tp) {_str, &dev_regs._var, (void*) _def, _tp},
DevRegInfo const devRegInfo[] = {
	VE_REGS

/* create a un32 for each module holding the enabled trace mask */
# define X(_name,_trc)	{"trace." #_name, &dev_regs.trace[VE_MOD_ ## _name], _trc, VE_UN32},
	VE_MODULES
# undef X

};
#undef XR

/// group values (output of AT command is limited)
DevRegGroup const devRegGroup[] =
{
	{DEV_REG_GRP_COMMON_B,	DEV_REG_GRP_COMMON_E,	"common"},
};

int const DEV_REG_GROUP_COUNT = sizeof(devRegGroup)/sizeof(devRegGroup[0]);

/** Check before the value is updated
 *
 * @return whether the change is allowed
 */
veBool dev_regBeforeChange(DevRegId regId, void const *value)
{
	ve_qtrace("regBeforeChange: regId %d", regId);
	return veTrue;
}

/** Additional action which should be performed after the
 *  register is changed.
 */
void dev_regOnChange(DevRegId regId, void const *value)
{
	ve_qtrace("regOnChange: regId %d", regId);
	switch (regId)
	{
	default:
		break;
	}
}
