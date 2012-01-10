#include <platform.h>
#include <errno.h>
#include <stdio.h>
#include <str.h>

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

static FILE* fopenname(char const *name, char const* mode)
{
	Str s;
	FILE *ret;
	str_newf(&s, "%s%s%s", "regs/", name, ".bin");
	if (s.error)
		return NULL;
	ret = fopen(s.data, mode);
	str_free(&s);
	return ret;
}

s8 adl_flhSubscribe(char *name, u16 dummy)
{
	if (adl_flhExist(name, 0) >= 0)
		return ERROR;
	return adl_flhWrite(name, 0, 0, NULL);
}

s32 adl_flhExist(char *name, u16 id)
{
	s32 ret;
	FILE* fh;
	fh = fopenname(name, "rb");
	if (!fh)
		return ERROR;
	fseek(fh, 0, SEEK_END);
	ret = ftell(fh);
	fclose(fh);
	return ret;
}

s8 adl_flhRead(char *name, u16 id, u16 len, u8 *buf)
{
	FILE* fh;
	fh = fopenname(name, "rb");
	if (!fh)
		return ERROR;
	if (fread(buf, 1, len, fh) != len) {
		fclose(fh);
		return ERROR;
	}
	fclose(fh);
	return OK;
}

s8 adl_flhWrite(char *name, u16 id, u16 len, u8 *buf)
{
	FILE* fh;
	fh = fopenname(name, "wb");
	if (!fh)
		return ERROR;
	if (fwrite(buf, 1, len, fh) != len) {
		fclose(fh);
		return ERROR;
	}
	fclose(fh);
	return OK;
}

s8 adl_flhErase(char *name, u16 id)
{
	remove(name);
	return OK;
}
