#ifndef _DEV_REG_APP_H_
#define _DEV_REG_APP_H_

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
#include <dev_reg.def.h>
#include <types.h>
#include <ve_trace.h>

#define DEV_REG_GRP_COMMON_B	0
#define DEV_REG_GRP_COMMON_E	(DEV_REG_COUNT - 1)

/* magic: build an enum for all register starting with DEV_REG_ */
#define XR(_enm,_str,_var,_def,_tp) DEV_REG_ ## _enm,
typedef enum {
	DEV_REG_UNKNOWN = -1,

	VE_REGS					/**< create the enum */

# define X(_name, _trc)		DEV_REG_TRACE_ ## _name,
	VE_MODULES
# undef X

	DEV_REG_COUNT
} DevRegId;
#undef XR

/**
 * Settings of the VGR
 * magic: this struct is build from dev_reg.def
 * trace settings are defined in ve_trace.def
 */
#define XR(_enm,_str,_var,_def,_tp) _tp ## _TP _var;
typedef struct
{
	u32 trace[VE_MOD_COUNT];	/**< flags to enable traces */
	VE_REGS						/**< create the others */
} DevRegisters;
#undef XR

extern DevRegisters dev_regs;

/* not so polite, but needs enum DevRegId which is app specific */
#include <dev_reg.h>

#endif

