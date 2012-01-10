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

#define VE_MOD 	VE_MOD_REG

#include <platform.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <dev_reg_app.h>
#include <ve_trace.h>
#include <ve_assert.h>

char const strEmpty[] = "";

/// The settings of this device
DevRegisters dev_regs;

/**
* @defgroup dev_reg Settings
* @ingroup vgr
* @brief Settings of the VGR
* @details
* The device registers is a global object shared between all modules.
* Members of this structure are loaded during startup or set to their
* default value when not encountered. The register are stored with the
* ADL Flash Service
*
* By default the registers are therefore remain stored during a software
* update. All registers can be removed from flash with the AT+WOPEN=3
* command.
* @{
*/

/**
* Loads and makes sure all registers exist. If any register does not yet exist, it is
* created, and initialised to the default value.
*/
static void initialiseRegisters(void)
{
	u16 regId;

	for (regId = 0; regId < DEV_REG_COUNT; regId++) {
		if (adl_flhSubscribe((char*) devRegInfo[regId].name, 1) == OK) {
			// This register was not previously subscribed, so initialise it.
			dev_regStore(regId, devRegInfo[regId].defaultVal);
		}
	}
}

/// Init Flash storage API
void dev_regInit(void)
{
	u16 regId;

	initialiseRegisters();

	// Load all registers from flash.
	for(regId = 0; regId < DEV_REG_COUNT; regId++)
		dev_regLoad(regId);
}

/** Returns a buffer which can store the value of the register as it is stored.
 *
 * The buffer uses the size of the object stored in flash, not the
 * length of the formatted string / the string currently in memory!
 *
 * @param regId the register to get the buffer for
 *
 * @note
 * 	Memory is dynamically allocated, use @ref ve_memRelease to free the memory.
 * 	returns \c NULL for non-existent registers.
 */
static char* dev_regAllocString(u16 regId)
{
	u16 length;
	char* ptr;
	char* handle = (char*) devRegInfo[regId].name;

	/* Size of strings is unknown, get size first */
	length = adl_flhExist(handle, 0);

	if (length <= 0)
		return NULL;

	ptr = ve_malloc(length);
	adl_flhRead(handle, 0, length, (u8*) ptr);
	ptr[length-1] = 0;

	return  ptr;
}

/// Load a register from the flash
veBool dev_regLoad(DevRegId regId)
{
	char* ptr;
	u16 length;
	char const* handle;

	ve_qtrace("regLoad: regId %d", regId);

	if (regId < 0 || regId >= DEV_REG_COUNT)
	{
		ve_error("regLoad: invalid registerId %d", regId);
		return veFalse;
	}

	handle = devRegInfo[regId].name;

	// dynamic string
	if(devRegInfo[regId].type == VE_STRING)
	{
		ptr = dev_regAllocString(regId);
		*((char**) devRegInfo[regId].dst) = ptr;
		return ptr != NULL;

	} else if ( (devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING)	{
		// fixed string
		u8 strLength = devRegInfo[regId].type & VE_FIXED_STRING_MASK ;
		length = adl_flhExist((char*) handle, 0);
		if (length > strLength)
			length = strLength;

		// make sure the string ends
		((char*) devRegInfo[regId].dst)[strLength] = '\0';
		return adl_flhRead((char*) handle, 0, length, devRegInfo[regId].dst) == OK;
	}

	switch (devRegInfo[regId].type)
	{
		case VE_UN8:
		case VE_SN8:
			length = 1;
			break;

		case VE_UN16:
		case VE_SN16:
			length = 2;
			break;

		case VE_UN32:
		case VE_SN32:
		case VE_INADDR:
			length = 4;
			break;

		default:
			ve_error("regStore: unknown type");
			return veFalse;
	}

	ve_qtrace("dev_regLoad: adl_flhRead length=%d 0x%x registerId %d", length,
				*(u32*) devRegInfo[regId].dst, regId);

	return adl_flhRead((char*) handle, 0, length, devRegInfo[regId].dst) == OK;
}

/**
* Save the current RAM value to flash. The RAM value is not reloaded.
*/
veBool dev_regSave(DevRegId regId)
{
	ve_qtrace("regSave: regId %d", regId);
	if (devRegInfo[regId].type == VE_STRING)
		return dev_regStore(regId, *((char**) devRegInfo[regId].dst));
	return dev_regStore(regId, devRegInfo[regId].dst);
}

/** Store value in flash. The RAM value is not reloaded.
*/
veBool dev_regStore(DevRegId regId, void const *value)
{
	u16 length = 0;
	s8 status;
	char* handle = (char*) devRegInfo[regId].name;
	u8 strLength;

	ve_qtrace("regStore: regId %d", regId);

	if (regId < 0 || regId >= DEV_REG_COUNT)
	{
		ve_error("regStore: invalid registerId %d", regId);
		return veFalse;
	}

	if (devRegInfo[regId].type == VE_STRING)
	{
		ve_qtrace("storing string: %s='%s'", handle,value);
		return adl_flhWrite(handle, 0, (u16) (strlen((char*) value) + 1), (u8*) value) == OK;
	}

	if ( (devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING)
	{
		// fixed string
		ve_qtrace ("storing fixed string %s='%s'", handle,value);

		strLength = devRegInfo[regId].type & VE_FIXED_STRING_MASK ;
		length = (u16) strlen((char*) value);
		if (length > strLength)
			return veFalse;

		// make sure the string ends
		return adl_flhWrite(handle, 0, length + 1, (u8*) value) == OK;
	}

	switch (devRegInfo[regId].type)
	{
		case VE_UN8:
		case VE_SN8:
			length = 1;
			break;

		case VE_UN16:
		case VE_SN16:
			length = 2;
			break;

		case VE_UN32:
		case VE_SN32:
		case VE_INADDR:
			length = 4;
			break;

		default:
			ve_error("regStore: unknown type, %d", devRegInfo[regId].type);
			return veFalse;
	}

	ve_qtrace ("storing register %d", regId);
	status = adl_flhWrite(handle, 0, length, (u8*) value);

	if(status != OK)
		ve_error("Unable to write %d bytes to register %d, because %d", length, regId, status);

	return status == OK;
}

/** Change regId to the new value pointed to by value
 *
 * The register is updated in memory as well as in flash
 * @param regId	id of the register
 * @param value the value to set, must be of the same type as the register
 *
 * @note
 * 		for dynamic string, VE_STRING, the new pointer in the register should
 * 		not be passed as the new value, since the current value will be released.
 * 		use @ref dev_regStore or @ref dev_regSave instead to save RAM to FLASH.
 *
 * @return whether successful
 */

veBool dev_regChange(DevRegId regId, void const *value)
{
	char* current;

	ve_qtrace("regChange: regId %d", regId);

	// release the current dynamic string if allocated
	if (devRegInfo[regId].type == VE_STRING)
	{
		current = *((char**) devRegInfo[regId].dst);
		if (current != NULL)
		{
			ve_assert(current != value);
			ve_free(current);
			current = NULL;
		}
	}

	if ( (devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING )
	{
		u8 strLength = devRegInfo[regId].type & VE_FIXED_STRING_MASK ;
		if (strlen(value) > strLength)
		{
			ve_error("attempt to set too low string, regId %d", regId);
			return veFalse;
		}
	}

	if ( !dev_regBeforeChange(regId, value) )
		return veFalse;

	if ( !dev_regStore(regId, value) )
		return veFalse;

	if ( !dev_regLoad(regId) )
		return veFalse;

	dev_regOnChange(regId, value);

	return veTrue;
}

/** Same as @ref dev_regChange, but accepts a string, also for numeric registers.
 *
 * @param regId	id of the register
 * @param value 	string containing the new value
 *
 * @note
 * 		for dynamic string, VE_STRING, the new pointer in the register should
 * 		not be passed as the new value, since the current value will be released.
 * 		use @ref dev_regStore or @ref dev_regSave instead to save RAM to FLASH.
 *
 *
 * @return whether successful
 */
veBool dev_regChangeString(DevRegId regId, char const *value)
{
	int ival;
	u8 dU8;
	u16 dU16;
	u32 dU32;

	ve_qtrace("regChangeString: regId %d", regId);

	// dynamic string, just forward
	if (devRegInfo[regId].type == VE_STRING)
		return dev_regChange(regId, value);

	// fixed string, just forward
	if ((devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING)
		return dev_regChange(regId, value);

	// Internet address specified in human-readable form -
	// check and convert it to inaddr
	if (devRegInfo[regId].type == VE_INADDR)
	{
		wip_in_addr_t inaddr;

		if (wip_inet_aton(value, &inaddr) == veFalse)
		{
			ve_error("regChangeString: invalid IP address specified \"%s\" for registerId %d", value, regId);
			return veFalse;
		}
		ve_qtrace("regChangeString: converted IP address \"%s\" to 0x%x", value, inaddr);

		return dev_regChange(regId, &inaddr);
	}

	errno = 0;
	ival = atoi(value);
	if (errno != 0)
		return veFalse;

	switch (devRegInfo[regId].type)
	{
		case VE_UN8:
		case VE_SN8:
			dU8 = (u8) ival;
			return dev_regChange(regId, &dU8);

		case VE_UN16:
		case VE_SN16:
			dU16 = (u16) ival;
			return dev_regChange(regId, &dU16);

		case VE_UN32:
		case VE_SN32:
			dU32 = (u32) ival;
			return dev_regChange(regId, &dU32);

		default:
			ve_error("dev_regChangeStr for unknown type, %d", devRegInfo[regId].type);
			return veFalse;
	}
}

static veBool getAsU32(DevRegId regId, u32* val)
{
	switch (devRegInfo[regId].type)
	{
	case VE_UN8:
		*val = (u32) *((u8*) devRegInfo[regId].dst);
		return veTrue;

	case VE_UN16:
		*val = (u32) *((u16*) devRegInfo[regId].dst);
		return veTrue;

	case VE_UN32:
		*val = *(u32*) devRegInfo[regId].dst;
		return veTrue;

	default:
		return veFalse;
	}
}

static veBool getAsS32(DevRegId regId, s32* val)
{
	switch (devRegInfo[regId].type)
	{
	case VE_SN8:
		*val = (s32) *((s8*) devRegInfo[regId].dst);
		return veTrue;

	case VE_SN16:
		*val = (s32) *((s16*) devRegInfo[regId].dst);
		return veTrue;

	case VE_SN32:
		*val = *(s32*) devRegInfo[regId].dst;
		return veTrue;

	default:
		return veFalse;
	}
}

/** Returns a formatted string of the register value
*
*  Buffer must be provided, @see dev_regValueStrLength.
*/
void dev_regValueString(DevRegId regId, char *tg)
{
	ve_qtrace("regValueString: regId %d", regId);

	if (devRegInfo[regId].type == VE_STRING) {
		strcpy(tg, *((char**) devRegInfo[regId].dst));
		return;
	}

	if ((devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING) {
		strcpy(tg, (char*) devRegInfo[regId].dst);
		return;
	}

	if (devRegInfo[regId].type == VE_INADDR) {
		wip_inet_ntoa(*(wip_in_addr_t *)devRegInfo[regId].dst, tg, 16);
		return;
	}

	{
		u32 val;
		if (getAsU32(regId, &val)) {
			sprintf(tg, "%lu", val);
			return;
		}
	}

	{
		s32 val;
		if (getAsS32(regId, &val)) {
			sprintf(tg, "%ld", val);
			return;
		}
	}

	*tg = 0;
	ve_error("dev_regValueStr for unknown type, %d", devRegInfo[regId].type);
}

/**
* returns the maximum string length of the formatted value of the registers
*
* @note the string length is return, not including the trailing \\0
*/

size_t dev_regValueStringLength(DevRegId regId)
{
	ve_qtrace("regValueStringLength: regId %d", regId);

	if (devRegInfo[regId].type == VE_STRING)
		return strlen(*((char**) devRegInfo[regId].dst));

	if ( (devRegInfo[regId].type & VE_FIXED_STRING) == VE_FIXED_STRING )
		return devRegInfo[regId].type & VE_BIT_MASK;

	switch (devRegInfo[regId].type)
	{
		case VE_UN8:
			return 3;

		case VE_SN8:
			return 4;	// -128

		case VE_UN16:
			return 5;	// 65535

		case VE_SN16:
			return 6;	// -32767

		case VE_UN32:
			return 10;	// 4,294,967,296

		case VE_SN32:
			return 11;

		case VE_INADDR:
			return 15;	// nnn.nnn.nnn.nnn

		default:
			ve_error("regValueStrLength for unknown type, %d", devRegInfo[regId].type);
			return 0;
	}
}

/**
* Get the formatted string of a value.
* @note return must be released with ve_free
*/
char* dev_regValue(DevRegId regId)
{
	char* ret;

	ve_qtrace("regValue: regId %d", regId);

	ret = ve_malloc((u32)(dev_regValueStringLength(regId) + 1));
	if (ret == NULL)
		return ret;

	dev_regValueString(regId, ret);
	return ret;
}

/**
* returns the name of the register
*/
char const* dev_regName(DevRegId regId)
{
	if (regId < 0 || regId >= DEV_REG_COUNT)
		return "UNKNOWN";

	return devRegInfo[regId].name;
}

/**
 * Determines the ID of a register from a string.  The string may contain either
 * the name or the number of the register.
 *
 * @param string	The string to process.
 *
 * @return The ID of the register.
 */
DevRegId dev_regIdFromString(char const* str)
{
	DevRegId i;

	if (!str)
		return DEV_REG_UNKNOWN;

	// Check to see if the string is a valid register number.
	errno = 0;
	i = atoi(str);
	if (!errno)
	{
		if (i < 0 || i >= DEV_REG_COUNT)
			return DEV_REG_UNKNOWN;
		return i;
	}

	/* Not a valid register number, check if it is a name.*/
	for (i = 0; i < DEV_REG_COUNT; i++)
		if (stricmp(str, devRegInfo[i].name) == 0)
			return i;

	return DEV_REG_UNKNOWN;
}

/**
* restore register to default
*/
veBool dev_regRestoreDefault(DevRegId regId)
{
	return dev_regChange(regId, devRegInfo[regId].defaultVal);
}

/**
 * Returns the register group identified by the supplied ID.
 *
 * @param id	The ID of the register group to retrieve.
 *
 * @return The register group or NULL if not found.
 */
DevRegGroup const* dev_regGroupFromId(int id)
{
	if (id >= DEV_REG_GROUP_COUNT)
		return NULL;

	return &devRegGroup[id];
}

/**
 * Returns the register group identified by the supplied string.  The string
 * can contain either the group's name or ID.
 *
 * @param string	The string identifying the register group to retrieve.
 *
 * @return The register group or NULL if not found.
 */
DevRegGroup const* dev_regGroupFromString(char const* str)
{
	u8 i;

	if (!str)
		return NULL;

	/* If string is a number, find the group with that ID. */
	errno = 0;
	i = atoi(str);
	if (!errno)
		return dev_regGroupFromId(i);

	// String is not a number, so find the group with that name.
	for (i = 0; i < DEV_REG_GROUP_COUNT; i++)
	{
		DevRegGroup const* group = dev_regGroupFromId(i);

		if (stricmp(str, group->name) == 0)
			return group;
	}

	// No matching group was found.
	return NULL;
}

/// @}
