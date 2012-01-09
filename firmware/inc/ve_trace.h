#ifndef _VE_TRACE_H_
#define _VE_TRACE_H_

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
#include <ve_trace.def.h>

/* magic: enum list from ve_trace.def */
#define X(_name,_trc)	VE_MOD_ ## _name,
typedef enum {
	VE_MODULES
	VE_MOD_COUNT
} VeModule;
#undef X

void ve_trace(VeModule module, u32 level, char const* format, ...);
void ve_tracen(VeModule module, u32 level, char const *prepend, char const *buf, int n);

#ifndef VE_MOD
#define VE_MOD VE_MOD_UNKNOWN
#endif

#define ve_error(format,...) ve_trace(VE_MOD,0,format,##__VA_ARGS__)
#define ve_warning(format,...) ve_trace(VE_MOD,1,format,##__VA_ARGS__)
#define ve_qtrace(format,...) ve_trace(VE_MOD,2,format,##__VA_ARGS__)
#define ve_ltrace(level,format,...) ve_trace(VE_MOD,level,format,##__VA_ARGS__)
#define ve_qtracen(p,b,n) ve_tracen(VE_MOD,2,p,b,n)
#define ve_ltracen(level,p,b,n) ve_tracen(VE_MOD,level,p,b,n)

void ve_traceEnableLevels(VeModule module, u32 mask);
void ve_traceDisableLevels(VeModule module, u32 mask);
void ve_traceEnable(VeModule module, u32 level);
void ve_traceDisable(VeModule module, u32 level);
void ve_traceEnableWarnings(void);
void ve_traceDisableWarnings(void);
void ve_traceEnableErrors(void);
void ve_traceDisableErrors(void);
void ve_tracesEnable(void);
void ve_tracesDisable(void);
veBool ve_traceLevelsStore(void);

veBool ve_traceEnabledExt(VeModule module, u32 level);
#define ve_traceEnabled(level) ve_traceEnabledExt(VE_MOD, level)

VeModule ve_traceModuleByName(char const* module);
char const* ve_traceModuleName(VeModule module);

#endif
