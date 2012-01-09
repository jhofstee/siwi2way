#ifndef _DEVREG_H_
#define _DEVREG_H_

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

#ifndef _DEV_REG_APP_H_
#error "include dev_reg_app.h"
#endif

/** Definition of a register */
typedef struct
{
	char const	*name;			/**< register name */
	void		*dst;			/**< pointer to actual value */
	void const	*defaultVal;	/**< pointer to default value */
	VeDatatype	type;			/**< data type of the register */
} DevRegInfo;

/// Definition of a group of settings
typedef struct
{
	DevRegId 	from;		///< The first register of this group.
	DevRegId 	till;		///< The last register of this group.
	char const*	name;		///< The name of this group.
} DevRegGroup;

extern char const strEmpty[];
extern DevRegGroup const devRegGroup[];
extern int const DEV_REG_GROUP_COUNT;

veBool				dev_regChange(DevRegId regId, void const *value);
veBool				dev_regChangeString(DevRegId regId, char const *value);
veBool				dev_regLoad(DevRegId regId);
DevRegId			dev_regIdFromString(char const *str);
void				dev_regInit(void);
char const * 		dev_regName(DevRegId regId);
veBool 				dev_regRestoreDefault(DevRegId regId);
veBool 				dev_regSave(DevRegId regId);
veBool				dev_regStore(DevRegId regId, void const *value);
void				dev_regValueString(DevRegId regId, char *tg);
size_t				dev_regValueStringLength(DevRegId regId);
char * 				dev_regValue(DevRegId regId);

DevRegGroup const*	dev_regGroupFromId(int id);
DevRegGroup const*	dev_regGroupFromString(char const* str);

/* internal */
# if VE_MOD == VE_MOD_REG
extern DevRegInfo const devRegInfo[];
veBool 				dev_regBeforeChange(DevRegId regId, void const *value);
void 				dev_regOnChange(DevRegId regId, void const *value);
# endif

#endif
