#ifndef _TYPES_TYPES_H_
#define _TYPES_TYPES_H_

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

#include <bindef.h>

#ifndef __OAT_API_VERSION__
#include <stdint.h>
typedef char s8;
typedef unsigned char u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
#endif

typedef s8			VE_SN8_TP;
typedef	u8			VE_UN8_TP;
typedef	s16			VE_SN16_TP;
typedef	u16			VE_UN16_TP;
typedef	s32			VE_SN32_TP;
typedef	u32			VE_UN32_TP;
typedef char*		VE_STRING_TP;
typedef u32			VE_INADDR_TP;

#define VE_REGULAR				b00000000
#define VE_BIT					b01000000
#define VE_EXCEPTIONAL			b10000000
#define VE_NON_VARIANT			b11000000
#define VE_FIXED_STRING_MASK	b00111111
#define VE_BIT_MASK				b00111111

typedef enum
{
	VE_UN8,
	VE_SN8,
	VE_UN16,
	VE_SN16,
	VE_UN32,
	VE_SN32,
	VE_FLOAT,
	VE_INADDR,  // Special type, requiring special processing to enter/show Internet
				// address in human-readable form, which is internally u32 integer

	VE_1BIT	= VE_BIT+1,
	VE_2BIT = VE_BIT+2,

	VE_STRING,
	VE_NMEA_STRING,
	VE_FIXED_STRING = VE_NON_VARIANT

} VeDatatype;

#endif

