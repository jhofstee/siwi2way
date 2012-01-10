#ifndef _STR_UTILS_H_
#define _STR_UTILS_H_

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

#ifdef _DEBUG
# define dmemset(dst, val, size)		memset(dst, val, size)
#else
# define dmemset(dst, val, size)
#endif

veBool		str_isVisible(char c);
veBool		str_isPrintable(char c);

#define		STR_UTILS_NEXT_WORD_START(a) ((a)+strlen(a)+1)
char*		str_word(char* SmsText, size_t length, char* curPtr);
#define  	str_nextWord(text, length, word) str_word(text, length, STR_UTILS_NEXT_WORD_START(word))

void 		str_printable(u8* data, u16 length, char* str);

// polite long names
#define 	str_atSendResponsePort(_t,_p,format,...) str_atSendResponse(ADL_AT_PORT_TYPE(_p,_t),format,##__VA_ARGS__);
s32 		str_atSendResponse(u16 Type, char const *format, ...);
s32 		str_atSendResponseVa(u16 Type, char const *format, va_list varArgs);

// shorter, but still clear
#define 	str_atPort(_p,format,...) str_atSendResponse(ADL_AT_PORT_TYPE(_p,ADL_AT_RSP),format,##__VA_ARGS__)
#define 	str_atPortInt(_p,format,...) str_atSendResponse(ADL_AT_PORT_TYPE(_p,ADL_AT_INT),format,##__VA_ARGS__)
#define 	str_atPortUns(_p,format,...) str_atSendResponse(ADL_AT_PORT_TYPE(_p,ADL_AT_UNS),format,##__VA_ARGS__)

int			str_phonePduFormat(char* number, char* formatted, char* firstChar);
u8			str_asciiToSms(u8 c);
u8			str_smsToAscii(u8 c);

void		str_asciiToPdu(char *text, size_t length, int startPos, char *pdu);
int 		str_get_next_phone(char *phone_num, char *phone_list, unsigned int pos);

void 		str_atOk(adl_port_e port);
void 		str_atError(adl_port_e port);
void 		str_atErrorTxt(adl_port_e port, char const *format, ...);
void 		str_atSuccess(adl_port_e port, veBool ok);

#endif
