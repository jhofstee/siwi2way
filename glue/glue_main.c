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

#include <conio.h>

#include <str.h>
#include <ve_at.h>
#include <ve_timer.h>

static void print_it(adl_atResponse_t* paras)
{
	printf("%s", paras->StrData);
}

void glue_main(void)
{
	Str cmd;
	str_new(&cmd, 512, 512);

	while (1)
	{
		while (kbhit()) {
			char c = getche();
			if (c == '\r') {
				fputc('\n', stdout);
				if (ve_atCmdSendExt(cmd.data, ADL_PORT_NONE, 0, NULL, print_it) != OK)
					printf("ERROR\n");
				str_set(&cmd, "");
			} else if (c == '\b') {
				str_rmc(&cmd);
				fputc(' ', stdout);
				fputc('\b', stdout);
			} else {
				str_addc(&cmd, c);
			}
		}
		wip_update(100);
		ve_timer_update();
	}
}
