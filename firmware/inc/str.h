#ifndef _STR_H_
#define _STR_H_

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
#include <types.h>

/// Simple string which dynamically allocates memory
typedef struct
{
	char *	data;			///< the actual string
	size_t	buf_size;		///< the buffer size
	size_t	step;			///< step size to increase the buffer with
	size_t	str_len;		///< length of the string in the buffer
	veBool	error;			///< indicates a failure of a operation on the string
} Str;

extern char const *str_invalid;

void str_new(Str *str, size_t size, size_t step);
void str_newf(Str *str, char const *format, ...);
void str_vnewf(Str *str, char const *format, va_list varArgs);
veBool str_newn(Str *str, char const* buf, size_t len);
size_t str_set(Str *str, char const* value);
char const * str_cstr(Str const *str);
size_t str_len(Str const *str);
void str_free(Str *str);

size_t str_add(Str *str, char const *value);
void str_addc(Str *str, char value);
void str_addInt(Str *str, int val);
void str_addIntStr(Str *str, int val, char const *post);
size_t str_addIntFmt(Str *str, int val, int len, char const *fmt);
size_t str_addIntExt(Str *str, int val, int dec);
void str_addf(Str *str, char const *format, ...);
void str_vaddf(Str *str, char const *format, va_list varArgs);
void str_addFloat(Str *str, float val, int dec);
void str_addFloatExt(Str *str, float val, int len, int dec, veBool fixed);
void str_addFloatStr(Str *str, float val, int dec, char const *post);
void str_addUrlEnc(Str *str, char const *string);
void str_addUrlEncChr(Str *str, char chr);

void str_addQVarNrStr(Str *str, char const *var, int n, char const* value);
void str_addQVarNrInt(Str *str, char const *var, int n, s32 value);
void str_addQVarNrFloat(Str *str, char const *var, int n, float value, int dec);
void str_addQVarArrNrFloat(Str *str, char const *var, int n, float value, int dec);
void str_addQVarStr(Str *str, char const *var, char const *value);
void str_addQVarInt(Str *str, char const *var, s32 value);
void str_addQVarFloat(Str *str, char const *var, float value, int dec);

void str_addcEscaped(Str *str, char c);
void str_addEscaped(Str *str, char const* s);
void str_addUnescaped(Str *str, char const *s);

void str_rmc(Str *str);

size_t str_printfLength(char const *format, va_list argptr);

#endif
