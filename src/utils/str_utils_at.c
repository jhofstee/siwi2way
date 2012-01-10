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

#include <stdarg.h>

#include "str.h"
#include "str_utils.h"
#include "ve_at.h"

/**
 * AT response sprintf with dynamic memory.
 */
s32 str_atSendResponseVa(u16 Type, char const *format, va_list varArgs)
{
	s32 ret;
	Str str;

	str_vnewf(&str, format, varArgs);
	ret = ve_atSendResponse(Type, str_cstr(&str));
	str_free(&str);

	return ret;
}

/**
 * AT response sprintf with dynamic memory.
 */
s32 str_atSendResponse(u16 Type, char const *format, ...)
{
	s32 ret;
	va_list varArgs;

	va_start(varArgs, format);
	ret = str_atSendResponseVa(Type, format, varArgs);
	va_end(varArgs);

	return ret;
}

/// send AT ok on port, just short version
void str_atOk(adl_port_e port)
{
	ve_atSendStdResponsePort(ADL_AT_RSP, port, ADL_STR_OK);
}

/// send AT error on port, short version
void str_atError(adl_port_e port)
{
	ve_atSendStdResponsePort(ADL_AT_RSP, port, ADL_STR_ERROR);
}

/// send at error on port, with sprintf like description
void str_atErrorTxt(adl_port_e port, char const *format, ...)
{
	va_list varArgs;

	va_start(varArgs, format);
	str_atSendResponseVa(ADL_AT_PORT_TYPE(port, ADL_AT_INT), format, varArgs);
	va_end(varArgs);

	str_atError(port);
}

/// other short, ok or error depending on flag
void str_atSuccess(adl_port_e port, veBool ok)
{
	if (ok)
		str_atOk(port);
	else
		str_atError(port);
}
