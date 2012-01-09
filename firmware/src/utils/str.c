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

/**
 * @defgroup Str Str
 * @ingroup utils
 * @{
 * @brief Simple string which dynamically allocates more memory if needed.
 *
 * Str.error will be set on failure and the data be freed.
 * Subsequent calls should not cause problems but silently fail, keeping
 * the error flag set. Checking at the end if the string is valid should
 * suffice (and it will likely not be reached, since the modem will
 * reset when it is out of memory).
 */

#include <stdarg.h>
#include <math.h>
#include <stdio.h>

#include <platform.h>
#include <str.h>
#include <ve_memory.h>
#include <ve_assert.h>

/* #define _DEBUG_STR */

#ifdef _DEBUG_STR
# define dbg_memset(dst, val, size)		memset(dst, val, size)
#else
# define dbg_memset(dst, val, size)
#endif

char const *str_invalid = "invalid str.";

/** Allocates a larger buffer and copies the existing string.
 *
 * @param str		the string
 * @param newSize	the new size of the buffer, must be larger
 *
 */
static void str_resize(Str *str, u16 newSize)
{
	char *tmp;

	if (str->error)
		return;

	if (str->step == 0)
	{
		str_free(str);
		return;
	}

#ifdef VE_REALLOC_MISSING
	// create new buffer
	tmp = (char*) ve_malloc(newSize);
	if (tmp == NULL)
	{
		str_free(str);
		return;
	}

	// copy contents
	dbg_memset(tmp, 0, newSize);
	if (str->data != NULL)
	{
		strcpy(tmp, str->data);
		ve_free(str->data);
	}

	str->data = tmp;
#else
	tmp = ve_realloc(str->data, newSize);
	if (tmp == NULL)
	{
		str_free(str);
		return;
	}
	str->data = tmp;
#endif

	str->buf_size = newSize;
}

/** Makes sure that there is space for lengthNeeded characters.
 *
 * @param str the string
 * @param lengthNeeded the length of the string to append
 */
static void str_fit(Str *str, size_t lengthNeeded)
{
	size_t spaceLeft;

	if (str->error)
		return;

	// if the string does not fit, allocate a multiple of step to make it fit
	spaceLeft = (u16) (str->buf_size - str_len(str) - 1);	// 1 for the ending 0
	if (spaceLeft < lengthNeeded)
	{
		if (str->step == 0)
		{
			str_free(str);
			return;
		}

		str_resize(str, (u16) ( str->buf_size + ((lengthNeeded - spaceLeft) / str->step + 1) * str->step));
	}
}

/** returns used length of the string. */
size_t str_len(Str const *str)
{
	if (str->error)
		return 0;
	return str->str_len;
}

/** Zero ended string or invalid string */
char const *str_cstr(Str const *str)
{
	if (str->error)
		return str_invalid;

	return str->data;
}

/** returns the end of the buffer or NULL if none
 * @param str the string to initialise
 */
static char *str_cur(Str const *str)
{
	if (str->data == NULL)
		return NULL;

	return str->data + str_len(str);
}

/* internal, call after chars are added */
static void str_added(Str *str, size_t chars)
{
	str->str_len += chars;
#ifdef _DEBUG_STR
	ve_assert(str->str_len == strlen(str->data));
#endif
}

/** Initialises the string.
 *
 * A buffer of length is allocated and zero ended or error flag is set.
 *
 * @param str 		the string to initialise
 * @param length 	initial length of the buffer
 * @param step 		number of bytes to expand the buffer with when needed
 *
 * @note should only be called once.
 * @note error flag is set when out of memory
 */
void str_new(Str *str, size_t size, size_t step)
{
	if (str == NULL)
		return;
	// create new buffer
	str->data = (char*) ve_malloc(size);
	if (str->data == NULL)
	{
		str->error = veTrue;
		return;
	}
	dbg_memset(str->data, 0, size);
	str->data[0] = 0;
	str->step = step;
	str->buf_size = size;
	str->str_len = 0;
	str->error = veFalse;
}

/**
 * sprintf with dynamic memory.
 *
 * @note the return value must be released!
 */
void str_vnewf(Str *str, char const *format, va_list varArgs)
{
	size_t length;

	/* get the length of the string */
	length = str_printfLength(format, varArgs);
	str_new(str, length + 1, 100);
	if (str->error)
		return;

	/* now it can safely be formatted */
	vsprintf(str->data, format, varArgs);
	str_added(str, length);
}

/**
 * sprintf with dynamic memory.
 *
 * @note the return value must be released!
 */
void str_newf(Str *str, char const *format, ...)
{
	va_list varArgs;

	// get the length of the string
	va_start(varArgs, format);
	str_vnewf(str, format, varArgs);
	va_end(varArgs);
}

veBool str_newn(Str *str, char const* buf, size_t len)
{
	str_new(str, len + 1, len + 1);
	if (str->error)
		return veFalse;
	memcpy(str->data, buf, len);
	str->data[len] = 0;
	str_added(str, len);
	return veTrue;
}

/** Releases the resources
 *
 * @param str the string to release
 */
void str_free(Str *str)
{
	ve_free(str->data);
	str->data = NULL;
	str->buf_size = 0;
	str->str_len = 0;
	str->error = veTrue;
}

/** Sets the string to a value.
 *
 * @param str the string
 * @param value the string to set
 */
size_t str_set(Str *str, char const *value)
{
	if (str->data == NULL)
		return 0;

	str->data[0] = 0;
	str->str_len = 0;
	dbg_memset(str->data, 0, str->buf_size);

	return str_add(str, value);
}

/** Appends a string.
 *
 * @param str the string
 * @param value the string to append
 */
size_t str_add(Str *str, char const *value)
{
	size_t len;
	char * p;

	if (value == NULL)
		return 0;

	len = strlen(value);
	str_fit(str, len);

	if (str->error)
		return 0;

	p = str_cur(str);
	memcpy(p, value, len);
	p[len] = 0;
	str_added(str, len);
	return len;
}

/** Appends a char.
 *
 * @param str the string
 * @param value the char to append
 */
void str_addc(Str *str, char value)
{
	char *p;

	str_fit(str, 1);
	if (str->error)
		return;

	p = str_cur(str);
	p[0] = value;
	p[1] = 0;
	str_added(str, 1);
}

/**
 * Appends an integer to the string with specified format.
 *
 * @param str the string
 * @param val the integer to add
 * @param len the length of the char of the integer, e.g. 3 for 100. (max 10, 2147483647)
 * @param fmt the formatter for sprintf
 *
 * @note one digit is added to the length to always allow negative numbers
 * @note if the number doesn't fit ### will be added instead.
 */
size_t str_addIntFmt(Str *str, int val, int len, char const *fmt)
{
	char invalid[] = "###########";
	float max = 1;
	int n;

	if (len > 10)
		return 0;

	n = len;
	while (n--)
		max *= 10;

	if (abs(val) >= max)
	{
		invalid[len] = 0;
		return str_add(str, invalid);
	}

	str_fit(str, len + 1);	// 1 for sign

	if (str->error)
		return 0;

	return sprintf(str_cur(str), fmt, val);
}

/**
 * Appends an integer to the string.
 *
 * @param str the string
 * @param val the integer to add
 * @param len the length of the char of the integer, e.g. 3 for 100. (max 9)
 *
 * @note one digit is added to the length to always allow negative numbers
 * @note if the number doesn't fit ### will be added instead.
 */
size_t str_addIntExt(Str *str, int val, int len)
{
	return str_addIntFmt(str, val, len, "%d");
}

/**
 * Appends an integer to the string.
 *
 * @param str the string
 * @param val integer to append
 *
 * @note one digit is added to the length to always allow negative numbers
 */
void str_addInt(Str *str, int val)
{
	str_addIntExt(str, val, 10);
}

/**
 * Appends an integer and string to the string.
 *
 * @param str the string
 * @param val integer to append
 * @param post the string to append
 */
void str_addIntStr(Str *str, int val, char const *post)
{
	str_addInt(str, val);
	str_add(str, post);
}

/** add string number of times
 *
 */
void str_repeat(Str *str, char const *value, int number)
{
	int n;
	for (n=0; n<number; n++)
		str_add(str, value);
}

/**
 * Appends a float to the string.
 *
 * @param str the string
 * @param val the float to add
 * @param len the length whole number part in char, e.g. 3 for 100. (max 9)
 * @param dec the number of decimals
 * @param fixed forces the specified size by padding with spaces
 *
 * @note one digit is added to the length to always allow negative numbers
 * @note if the number doesn't fit ### will be added instead.
 */
void str_addFloatExt(Str *str, float val, int len, int dec, veBool fixed)
{
	char format[20];
	float frac, absval;
	int fracint;
	int n;
	int maxFrac = 1;
	size_t numchars = 0;
	size_t totalLength;

	totalLength = (size_t) (len + dec + 1);

	// NOTE: %f isn't supported, this must be implemented by integers!

	// determine whole number and decimals
	absval = (float) fabs(val);
	frac = (float) fabs(val);
	frac -= (int) absval;
	for (n=0; n<dec; n++)
	{
		frac *= 10;
		maxFrac *= 10;
	}

	// then round the decimal itself, and if necessary the whole number
	// e.g. 0.999999... should always become 1.0
	fracint = (int) (frac + 0.5);
	if (fracint >= maxFrac)
	{
		absval += 1;
		fracint = 0;
	}

	// only then display... force minus when negative, for e.g. -0.95
	// but not -0.00
	if (val < 0 &&  !((int) absval == 0 && fracint == 0) )
	{
		numchars++;
		str_add(str, "-");
	}

	// report whole number
	numchars += str_addIntExt(str, (int) absval, len);
	if (dec == 0)
	{
		if (fixed)
			str_repeat(str, " ", (int) (totalLength - numchars));
		return;
	}

	// report decimals as well
	str_add(str, ".");
	sprintf(format, "%%0%dd", dec);
	numchars += str_addIntFmt(str, fracint, dec, format);

	if (fixed)
		str_repeat(str, " ", (int) (totalLength - numchars));
}

/**
 * Appends a float to the string.
 *
 * @param str the string
 * @param val the float to add
 * @param dec the number of decimals
 *
 * @note one digit is added to the length to always allow negative numbers
 */
void str_addFloat(Str *str, float val, int dec)
{
	str_addFloatExt(str, val, 10, dec, veFalse);
}

/**
 * Appends a float to the string.
 *
 * @param str the string
 * @param val the float to add
 * @param dec the number of decimals
 * @param post string to append after the number (e.g. it unit)
 *
 * @note one digit is added to the length to always allow negative numbers
 */
void str_addFloatStr(Str *str, float val, int dec, char const *post)
{
	str_addFloatExt(str, val, 10, dec, veFalse);
	str_add(str, post);
}

/**
 * This function behaves like sprintf, except that it appends to a Str.
 *
 * @param str		The Str to which the formatted string will be appended.
 * @param format	The printf style format string for the output.
 * @param ...		Any additional args required by format.
 *
 * @retval	TRUE	The output was appended.
 * @retval	FALSE	It was not possible to append the output.
 */
void str_vaddf(Str *str, char const *format, va_list varArgs)
{
	size_t length;

	/* get the length of the string */
	length = str_printfLength(format, varArgs);

	/* make sure there is memory enough */
	str_fit(str, length);
	if (str->error)
		return;

	/* now it can safely be added */
	vsprintf(str_cur(str), format, varArgs);
	str_added(str, length);
}

void str_addf(Str *str, char const *format, ...)
{
	va_list varArgs;
	va_start(varArgs, format);
	str_vaddf(str, format, varArgs);
	va_end(varArgs);
}

/**
 * Add char urlencoded
 */
void str_addUrlEncChr(Str *str, char chr)
{
	/* This is not complete, just added when needed */
	if (chr == '#' || chr == '/' || chr == '-' || chr == '_' || isalnum(chr)) {
		str_addc(str, chr);
		return;
	}

	str_addf(str, "%%%02X", chr);
}

/**
 * Add string urlencoded
 */
void str_addUrlEnc(Str *str, char const *string)
{
	size_t n;

	for (n = 0; n < strlen(string); n++)
		str_addUrlEncChr(str, string[n]);
}

/**
 * Adds a variable with a numeric suffix and its value to
 * the string (&varN=value)
 *
 * @note var is not urlencoded, value is
 */
void str_addQVarNrStr(Str *str, char const *var, int n, char const *value)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addInt(str, n);
	str_addc(str, '=');
	str_addUrlEnc(str, value);
}

/**
 * Adds a variable with a numeric suffix and its value to
 * the string (&varN=value)
 *
 * @note var and value are not urlencoded
 */
void str_addQVarNrInt(Str *str, char const *var, int n, s32 value)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addInt(str, n);
	str_addc(str, '=');
	str_addInt(str, value);
}

/**
 * Adds a variable with a numeric suffix and its value to
 * the string (&varN=value)
 *
 * @note var is not urlencoded
 */
void str_addQVarNrFloat(Str *str, char const *var, int n, float value, int dec)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addInt(str, n);
	str_addc(str, '=');
	str_addFloat(str, value, dec);
}

/**
 * Adds an array variable with a numeric suffix and its value to
 * the string (&varN[]=value)
 *
 * @note var is not urlencoded
 */
void str_addQVarArrNrFloat(Str *str, char const *var, int n, float value, int dec)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addInt(str, n);
	str_add(str, "[]=");
	str_addFloat(str, value, dec);
}

/** Adds a variable and value to the string (&var=value)
 *
 * @note var is not urlencoded, value is
 */
void str_addQVarStr(Str *str, char const *var, char const *value)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addc(str, '=');
	str_addUrlEnc(str, value);
}

/** Adds a variable and value to the string (&var=value)
 *
 * @note var is not urlencoded
 */
void str_addQVarInt(Str *str, char const *var, s32 value)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addc(str, '=');
	str_addInt(str, value);
}

/** Adds a variable and value to the string (&var=value)
 *
 * @note var is not urlencoded
 */
void str_addQVarFloat(Str *str, char const *var, float value, int dec)
{
	str_addc(str, '&');
	str_add(str, var);
	str_addc(str, '=');
	str_addFloat(str, value, dec);
}

/**
 * Returns the number of characters in the string after formatting. 
 * Not including the trailing zero.
 *
 * @param format The format string that will be used for the output.
 * @param argptr The arguments to format.
 *
 * @retval The length of the printf output (excluding null terminator).
 */
size_t str_printfLength(char const *format, va_list argptr)
{
#if defined(WIN32) && !(defined(__MINGW32__) || defined(__MINGW64__))
	/* The MSVC compiler provides the _vscprintf for this purpose. */
	return _vscprintf(format, argptr);
#else
	/*
	 * The ARM_ELF_GCC compiler will return the number for characters
	 * required for the full output of the *nprintf functions, even if the
	 * maximum number of characters to write is specified too low.  However,
	 * the null terminator is always written, so a valid buffer must be
	 * provided even when specifying that 0 characters should be written.
	 */
	char dummy;
	return vsnprintf(&dummy, 0, format, argptr);
#endif
}

void str_addcEscaped(Str *str, char c)
{
	switch(c)
	{
	case '\n':
		str_add(str, "\\n");
		break;
	case '\r':
		str_add(str, "\\r");
		break;
	case '"':
		str_add(str, "\\\"");
		break;
	case '\'':
		str_add(str, "\\'");
		break;
	case '\\':
		str_add(str, "\\\\");
		break;
	default:
		str_addc(str, c);
		break;
	}
}

void str_addEscaped(Str *str, char const* s)
{
	while(*s && !str->error)
		str_addcEscaped(str, *s++);
}

void str_addUnescaped(Str *str, char const *s)
{
	str_fit(str, strlen(s));

	while(*s != 0 && !str->error)
	{
		if (*s == '\\')
		{
			switch (*++s)
			{
			case 'n':
				str_addc(str, '\n');
				break;
			case 'r':
				str_addc(str, '\r');
				break;
			case '"':
				str_addc(str, '"');
				break;
			case '\'':
				str_addc(str, '\'');
				break;
			case '\\':
				str_addc(str, '\\');
				break;
			case 0:
				return;
			default:
				str_addc(str, *s);
				break;
			}
		} else {
			str_addc(str, *s);
		}

		s++;
	}
}

void str_rmc(Str *str)
{
	if (!str->str_len)
		return;
	str->str_len--;
	str->data[str->str_len] = 0;
}

// @}
