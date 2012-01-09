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

#include <stdarg.h>
#include <string.h>

#include <dev_reg_app.h>
#include <str.h>
#include <ve_at.h>
#include <ve_trace.h>

struct QueueStr {
	Str str;
	struct QueueStr* next;
};

static struct QueueStr *queue;
static struct QueueStr *tail;
static adl_tmr_t *tmr;
u32 const defaultTrace = 0;

/**
 * @defgroup ve_trace Traces
 * @ingroup utils
 * @brief Support for runtime debugging
 * @{
 */

/* preprocessor magic: array with module names from ve_trace.def */
#define X(_name, _trc) #_name,
const char * modules[] = {
	VE_MODULES
};
#undef X

static void retry(u8 id, void *ctx);

static void try_to_send(void)
{
	while(queue)
	{
		if (ve_atSendResponsePort(ADL_AT_UNS, (u8) dev_regs.tracePort, queue->str.data) == OK)
		{
			struct QueueStr* q = queue;
			queue = queue->next;
			if (!queue)
				tail = NULL;
			str_free(&q->str);
			ve_free(q);
		} else {
#ifndef _WIN32
			// schedule timer
			if (!tmr)
				tmr = adl_tmrSubscribe(veFalse, 1, ADL_TMR_TYPE_TICK, retry);
#endif
			return;
		}
	}
}

static void retry(u8 id, void *ctx)
{
	tmr = NULL;
	try_to_send();
}

/** printf like tracing, can be send to serial or stored in memory.
 * configurable by AT commands.
 */
void ve_trace(VeModule module, u32 level, char const* format, ...)
{
	va_list varArgs;
	struct QueueStr* q;
	Str* s;

	// check sane arguments
	if (level > 31) {
		adl_atSendResponse(ADL_AT_UNS, "\r\n+VERR:\"invalid trace level!\"");
		return;
	}

	// check sane arguments
	if (module > VE_MOD_COUNT) {
		adl_atSendResponse(ADL_AT_UNS, "\r\n+VERR:\"invalid trace module!\"");
		return;
	}

	if (!ve_traceEnabledExt(module, level))
		return;

	q = ve_malloc(sizeof(struct QueueStr));
	if (!q)
		return;
	s = &q->str;

	// trace level 0 = error, 1 = warning, other is unspecified
	str_new(s, 100, 100);
	if (level == 0)
		str_add(s, "+VERR: \"");
	else if (level == 1)
		str_add(s, "+VWRN: \"");
	else
		str_add(s, "+VIND: \"");

#ifndef _WIN32
	{
		adl_rtcTime_t 	now;
		adl_rtcGetTime(&now);
		str_addf(s, "%02d/%02d/%04d %02d:%02d:%02d",
			now.Day, now.Month,	now.Year,
			now.Hour, now.Minute, now.Second);
	}
#endif

	str_add(s, "\",\"");
	str_add(s, modules[module]);
	str_add(s, "\",");
	str_addInt(s, level);
	str_add(s, ",\"");

	va_start(varArgs, format);
	str_vaddf(s, format, varArgs);
	va_end(varArgs);

	str_add(s, "\"\r\n");

	if (s->error) {
		ve_free(q);
		return;
	}

	q->next = NULL;
	if (queue)
		tail->next = q;
	else
		queue = q;
	tail = q;

	try_to_send();
}

void ve_tracen(VeModule module, u32 level, char const *prepend, char const *buf, int n)
{
	int i;
	Str s;

	return;

	if (!ve_traceEnabledExt(module, level) || n <= 0)
		return;
	str_new(&s, 256, 256);
	str_set(&s, prepend);
	for (i = 0; i < n; i++) {
		if (buf[n] == '\n') {
			ve_qtrace("%s", s.data);
			str_set(&s, prepend);
		}
		str_addcEscaped(&s, buf[i]);
	}
	if (str_len(&s))
		ve_trace(module, level, "%s", s.data);
	str_free(&s);
}

/// enable the levels specified by the mask, leaving the others untouched
void ve_traceEnableLevels(VeModule module, u32 mask)
{
	dev_regs.trace[module] |= mask;
}

/// disables the levels specified by the mask, leaving the others untouched
void ve_traceDisableLevels(VeModule module, u32 mask)
{
	dev_regs.trace[module] &= ~mask;
}

/// enable a single trace
void ve_traceEnable(VeModule module, u32 level)
{
	ve_traceEnableLevels(module, 1 << level);
}

/// disable a single trace
void ve_traceDisable(VeModule module, u32 level)
{
	ve_traceDisableLevels(module, 1 << level);
}

/// enable all warnings
void ve_traceEnableWarnings(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceEnable(n, 1);
}

/// disable all warnings
void ve_traceDisableWarnings(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceDisable(n, 1);
}

/// enable all errors
void ve_traceEnableErrors(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceEnable(n, 0);
}

/// disable all errors
void ve_traceDisableErrors(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceDisable(n, 0);
}

///enable all traces
void ve_tracesEnable(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceEnableLevels(n, 0xFFFFFFFF);
}

/** Disable all traces
 */
void ve_tracesDisable(void)
{
	int n;
	for (n=0; n < VE_MOD_COUNT; n++)
		ve_traceDisableLevels(n, 0xFFFFFFFF);
}

/// returns the state of a single trace
veBool ve_traceEnabledExt(VeModule module, u32 level)
{
	return dev_regs.trace[module] & (1 << level) ? veTrue : veFalse;
}

/// Store the trace levels, so they are kept over a power cycle
veBool ve_traceLevelsStore(void)
{
	DevRegId regId;
	for (regId = DEV_REG_TRACE_NONE; regId < DEV_REG_TRACE_NONE + VE_MOD_COUNT; regId++) {
		if (!dev_regSave(regId))
			return veFalse;
	}

	return veTrue;
}

/// get the module id by the name
VeModule ve_traceModuleByName(char const *module)
{
	int n;

	if (stricmp(module, "all") == 0)
		return VE_MOD_ALL;

	for (n=0; n < VE_MOD_COUNT; n++)
	{
		if (stricmp(module, (char*) modules[n]) == 0)
			return n;
	}
	return VE_MOD_NONE;
}

/// get the module name by id
char const* ve_traceModuleName(VeModule module)
{
	return modules[module];
}

/// @}
