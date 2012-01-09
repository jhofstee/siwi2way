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
#include <ve_timer.h>
#include <ve_trace.h>

/**
 * A simple, one shot, inaccurate timer which, unlike adl_tmrSubscribe, 
 * cannot fail to subscribe. The VeTimer must be preserved till the
 * callback or till cancelation.
 *
 * static VeTimer tmr;
 * ve_timer(&tmr, 5, some_callback, ptr);
 *
 * to cancel call cancel or set zero timeout
 * ve_timer_cancel(&tmr);
 * ve_timer(&tmr, 0, NULL, NULL);
 *
 * to reload the timer just call again
 * ve_timer(&tmr, 15, some_callback, ptr);
 *
 * to create a periodic timer, set the timer again in the callback.
 *
 * @note Don't copy VeTimer objects! Their address is used to identify them.
 * @note Not fully tested yet!
 */

struct VeTimer* queue;

void ve_timer(struct VeTimer* tmr, u32 sec, VeTimerCallback cb, void *ctx)
{
	struct VeTimer *p = queue;

	if (!sec) {
		ve_timer_cancel(tmr);
		return;
	}

	tmr->value = sec;
	tmr->cb = cb;
	tmr->ctx = ctx;

	/* already enqueued? */
	while (p) {
		if (p == tmr)
			return;
		p = p->next;
	}

	/* nope, add it */
	tmr->next = queue;
	queue = tmr;
}

void ve_timer_cancel(struct VeTimer* tmr)
{
	struct VeTimer *p = queue;
	struct VeTimer *prev = NULL;

	/* already enqueued? */
	while (p) {
		if (p == tmr)
			break;
		prev = p;
		p = p->next;
	}

	if (!p) {
		ve_ltrace(17, "ve_timer_cancel timer not found %p", tmr);
		return;
	}

	if (prev)
		prev->next = p->next;
	else
		queue = p->next;
	p->next = NULL;
}

void ve_timer_tick(void)
{
	struct VeTimer *tmr = queue;
	struct VeTimer *prev = NULL;
	struct VeTimer *next;

	while (tmr) {
		next = tmr->next;
		if (--tmr->value == 0)
		{
			/* dequeue */
			if (prev)
				prev->next = tmr->next;
			else
				queue = tmr->next;
			tmr->next = NULL;

			/* call */
			if (tmr->cb)
				tmr->cb(tmr->ctx);

		} else {
			prev = tmr;
		}
		tmr = next;
	}
}

#if defined(__OAT_API_VERSION__)

static void stub(u8 ID, void *ctx)
{
	ve_timer_tick();
}

void ve_timer_init(void)
{
	adl_tmrSubscribe(veTrue, 10, ADL_TMR_TYPE_100MS, stub);
}

#elif defined(_WIN32)

#pragma comment(lib, "winmm.lib")

#include <mmsystem.h>
static HANDLE tickEv;

void ve_timer_init(void)
{
	tickEv = CreateEvent(NULL, veFalse, veFalse, TEXT("TimerEvent")); 
	timeSetEvent(1000, 100, (LPTIMECALLBACK) tickEv, (DWORD_PTR) NULL,
								TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
}

void ve_timer_update(void)
{
	if (WaitForSingleObject(tickEv, 0) == WAIT_OBJECT_0)
		ve_timer_tick();
}

#endif
