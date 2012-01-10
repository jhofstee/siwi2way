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

#include <ve_at.h>
#include <platform.h>
#include <ve_assert.h>
#include <ve_trace.h>

/**
 * @defgroup ve_at AT forwarding
 * @ingroup utils
 * @brief
 * 	Victron version of adl_AT allowing AT+V from the code itself.
 * @details
 *
 * The adl_at commands do not allow calling our own created AT commands.
 * This is annoying since a non physical external interface
 * (TCP/IP, SMS, LUA etc) can thus never access them.
 *
 * The implemented way around this is by explicitly checking if the command
 * is local implemented or a standard AT command. Local recognised commands
 * are directly called, which directly calls this module again, which directly
 * call the original caller.
 *
 * As a consequence:
 *	Don't call other local AT from local AT commands!
 *
 * @note
 * 	Naming is a bit confusing
 *		- cmdSubscribe  The callback is called when the AT command is issued
 *		- cmdCreate  	The specified AT is execute and the response is returned
 *		- cmdSend		Same but without the Port to execute it on..
 *		- cmdSendExt	Same but with port again, but without rspFlags
 *
 * @note
 * 	This is not a complete implementation:
 * 		- Unsolicited responses are not supported
 *		- Local commands can not be unsubscribed (but we don't do that anyway..)
 *
 * @{
 */

/// Structure to hold the ATV commands which should be locally available.
typedef struct VeAtCmdSubcribeS
{
	char *atCmd;					///< AT command which is implemented in the VGR
	adl_atCmdHandler_t cmdHndl;		///< Handler for processing the AT command
	u16 cmdOpt;						///< number of arguments etc
	struct VeAtCmdSubcribeS *next;
} VeAtCmdSubcribe;

static adl_atCmdPreParser_t*	ve_atAllocCmdPreParser(char const *str);
static s8						ve_atCmdParse(adl_atCmdPreParser_t *paras, char const *atcmd);
static void						ve_atFreeCmdPreParser(adl_atCmdPreParser_t *paras);

/// list with locally available ATV commands
static VeAtCmdSubcribe* ve_atSubsribedList = NULL;

/*
 * Since AT+V commands are blocking there can only be one at a
 * time and only one instance is needed..
 */
static adl_atRspHandler_t	ve_atRspHandler;
static void					*ve_atRspContext;

static void add_cmd(VeAtCmdSubcribe *cmd)
{
	VeAtCmdSubcribe* p = ve_atSubsribedList;

	if (p) {
		while (p->next)
			p = p->next;
	}

	cmd->next = NULL;
	if (p)
		p->next = cmd;
	else
		ve_atSubsribedList = cmd;
}

/**
 * @brief
 * 	Subscribe a custom AT command which should also be locally available.
 * @details
 *  "Subscribing" means that the caller implements the AT command in the handler.
 */
s16 ve_atCmdSubscribe(char const *cmdStr, adl_atCmdHandler_t cmdHdl, u16 cmdOpt)
{
	s16 ret;
	VeAtCmdSubcribe* cmd;

	// register as new AT command
	if ((ret = adl_atCmdSubscribe((char*) cmdStr, cmdHdl, cmdOpt)) != OK)
		return ret;

	if (!(cmd = (VeAtCmdSubcribe*) ve_malloc(sizeof(*cmd))))
	{
		ret = ERROR;
		goto error;
	}

	// get space for the at command
	if (!(cmd->atCmd = ve_malloc((u32) (strlen(cmdStr) + 1))))
	{
		ret = ERROR;
		goto error;
	}

	strcpy(cmd->atCmd, cmdStr);
	cmd->cmdOpt = cmdOpt;
	cmd->cmdHndl = cmdHdl;
	add_cmd(cmd);
	return OK;

error:
	adl_atCmdUnSubscribe((char*) cmdStr, cmdHdl);
	if (cmd)
		ve_free(cmd->atCmd);
	ve_free(cmd);
	return ret;
}

/**
 * Check if the command is locally implemented, ie locally subscribed.
 */
static VeAtCmdSubcribe* ve_atCmdSubscribeFindNext(char* atstr, VeAtCmdSubcribe* from)
{
	VeAtCmdSubcribe *ret = (from ? from->next : ve_atSubsribedList);

	while (ret)
	{
		if (strnicmp(ret->atCmd, atstr, (u32) strlen(ret->atCmd)) == 0)
			return ret;
		ret = ret->next;
	}

	return NULL;
}

/**
 * @brief
 *  Query or execute an AT command, all responses are sent back to the issuing
 *  port.
 *
 * @details
 * 	Overwrite of adl_atCmdSendExt also allowing execution of application
 *  specific AT commands.
 *
 * @note
 * 	Filtering is not implemented.
 *
 * @note
 *  Recursion is limited to one level (a custom AT command may only execute
 *  another custom AT command if that command does not execute other custom
 *  AT commands.
 *
 * @note
 *  Recursive calls can only be executed on a terminal (i.e. not from the SMS
 *  handler or other internal parser).
 *
 * @param atstr		The AT command to execute.
 * @param port		Not used.
 * @param NI		Not used.
 * @param Contxt    A context that will be passed to the response handler.
 * @param rsphdl	The handler for responses generated by this command.
 *
 * @retval	OK The command was executed successfully (note, this does not
 * 			reflect	the return status of the command itself).
 * @retval	ADL_RET_ERR_SERVICE_LOCKED The command could not be executed because
 * 			it was called from a forbidden context (low level interrupt, or too
 * 			many levels of recursion).
 * @retval	ERROR The command could not be executed because of a lack of
 * 			resources.
 */
s8 ve_atCmdSendExt(char *atstr, adl_atPort_e port, u16 ni, void* ctx, adl_atRspHandler_t rsphdl)
{
	VeAtCmdSubcribe *cmd = NULL;
	s8 ret = OK;
	veBool found = veFalse;
	adl_atCmdPreParser_t* preParser = NULL;

	// Do not nest..
	if (port == ADL_PORT_VICTRON)
		return ADL_RET_ERR_SERVICE_LOCKED;

	// Check if there is at least one application implementation.
	if (!(cmd = ve_atCmdSubscribeFindNext(atstr, cmd)))
	{
		ret = adl_atCmdSendExt(atstr, port, ni, ctx, rsphdl, "*", NULL);
		goto cleanup;
	}

	// note global...
	ve_atRspContext = ctx; 
	ve_atRspHandler = rsphdl;

	// create the parse object
	preParser = ve_atAllocCmdPreParser(atstr);
	if (preParser == NULL)
	{
		ret = ERROR;
		goto cleanup;
	}

	// actually parse it
	ret = ve_atCmdParse(preParser, cmd->atCmd);
	if (ret != OK)
		goto cleanup;

	// forward it
	do {
		if (	(cmd->cmdOpt & preParser->Type) == 0 || (
					preParser->Type == ADL_CMD_TYPE_PARA &&	(
						preParser->NbPara < (cmd->cmdOpt & 0x0F) ||		// min
						preParser->NbPara > ((cmd->cmdOpt & 0xF0) >> 4)	// max
					)
				)
			)
			continue;

		found = veTrue;
		cmd->cmdHndl(preParser);
		break;
	} while ((cmd = ve_atCmdSubscribeFindNext(atstr, cmd)) != NULL);

	if (!found)
	{
		ret = adl_atCmdSendExt(atstr, port, ni, ctx, rsphdl, "*", NULL);
		goto cleanup;
	}

	if (ve_atRspHandler != NULL)
	{
		ve_error("Fatal, command did not send final response!");
		ve_assert(veFalse);
	}

cleanup:
	// Callback not invoked, invalid format etc, cleanup...
	ve_atFreeCmdPreParser(preParser);
	ve_atRspHandler = NULL;
	ve_atRspContext = NULL;
	return ret;
}

/**
 * Forward the response of a custom AT command to the local handler who wants
 * to receive the response.
 */
static void ve_atForwardResponse(adl_atResponse_t* paras)
{
	// figure out who is interested in this
	if (ve_atRspHandler)
		ve_atRspHandler(paras);

	if (paras->IsTerminal)
	{
		ve_atRspHandler = NULL;
		ve_atRspContext = NULL;
	}
}

/**
 * Overwrite of adl_atSendStdResponse, also supporting internal AT+V commands.
 */
s32 ve_atSendStdResponse(u16 Type, adl_strID_e RspID)
{
	char *str;
	adl_atResponse_t* paras;

	if ( (Type >> 8) == ADL_PORT_VICTRON )
	{
		// get the string corresponding the ID
		str = adl_strGetResponse(RspID);
		if (str == NULL)
			return ERROR;

		// create a parser object with the string
		paras = ve_atAllocAtResponse(str, veTrue, Type);
		adl_memRelease(str); // note adl memory!

		if (paras == NULL)
			return ERROR;

		// copy some known information
		paras->RspID = RspID;

		// actual forwarding of the data
		ve_atForwardResponse(paras);

		// cleanup
		ve_free(paras);

		return OK;
	}

	return adl_atSendStdResponse(Type, RspID);
}

/**
 * Override of adl_atSendResponse, also supporting internal AT+V commands.
 */
s32 ve_atSendResponse(u16 Type, char const *Text)
{
	if ( (Type >> 8) == ADL_PORT_VICTRON )
	{
		adl_atResponse_t* paras;

		// create a parser object from the string
		paras = ve_atAllocAtResponse(Text, veFalse, Type);

		if (paras == NULL)
			return ERROR;

		// copy some known information
		paras->RspID = ADL_STR_NO_STRING;

		// actual forwarding of the data
		ve_atForwardResponse(paras);

		ve_free(paras);
		return OK;
	}

	return adl_atSendResponse(Type, (char*) Text);
}

/**
 * @note
 *	return object must be released with ve_free..
 */
adl_atResponse_t* ve_atAllocAtResponse(char const *str, veBool addCrLf, u16 Type)
{
	adl_atResponse_t* ret;
	size_t length;

	length = strlen(str);
	if (addCrLf)
		length += 2;

	// string is appended, \0 is already in adl_atCmdPreParser_t
	ret = (adl_atResponse_t*) ve_malloc((u32) (sizeof(adl_atResponse_t) + length));
	if (ret == NULL)
		return NULL;

	// copy string
	ret->StrLength = (u16) length;
	strcpy(ret->StrData, str);

	if (addCrLf)
	{
		ret->StrData[length-2] = '\r';
		ret->StrData[length-1] = '\n';
		ret->StrData[length] = 0;
	}

	// set some defaults
	ret->Dest = (Type >> 8);
	ret->IsTerminal = ( Type & 0xFF ) == ADL_AT_RSP;
	ret->Contxt = ve_atRspContext;

	return ret; // don't forget to release!
}

/**
 * @note
 *	return object must be released with ve_atFreeCmdPreParser..
 */
adl_atCmdPreParser_t* ve_atAllocCmdPreParser(char const *atstr)
{
	// string is appended, \0 is already in adl_atCmdPreParser_t
	adl_atCmdPreParser_t* ret = (adl_atCmdPreParser_t*) ve_malloc(
			(u32) (sizeof(adl_atCmdPreParser_t) + strlen(atstr))
		);

	if (ret == NULL)
		return NULL;

	ret->StrLength = (u16) strlen(atstr);
	strcpy(ret->StrData, atstr);

	// no parsed parameters (yet)
	ret->ParaList = NULL;
	ret->NbPara = 0;

	// set some defaults
	ret->NI = 0;
	ret->Contxt = ve_atRspContext;
	ret->Port = ADL_PORT_VICTRON;
	ret->Type = 0;  // no idea yet

	return ret; // don't forget to release!
}

#ifdef __OAT_API_VERSION__

static char* parseParamsInit(adl_atCmdPreParser_t *paras, char const * str)
{
	static wm_lstTable_t freeit = {NULL, ve_free};
	paras->ParaList = wm_lstCreate(WM_LIST_NONE, &freeit);
	if (!paras->ParaList)
		return NULL;
	return ve_strdup(str);
}

static veBool parseParamsAdd(adl_atCmdPreParser_t *paras, char const * str)
{
	char* q = ve_strdup(str);
	if (!q)
		return veFalse;
	wm_lstAddItem(paras->ParaList, (void*) q);
	paras->NbPara++;
	return veTrue;
}

static void parseParamsCleanup(char * str)
{
	ve_free(str);
}

static void ve_atFreeCmdPreParser(adl_atCmdPreParser_t* paras)
{
	if (paras && paras->ParaList)
		wm_lstDestroy(paras->ParaList);
	ve_free(paras);
}
#else

static char* parseParamsInit(adl_atCmdPreParser_t *paras, char const * str)
{
	return (paras->ParaList = ve_strdup(str));
}

static veBool parseParamsAdd(adl_atCmdPreParser_t *paras, char const * str)
{
	if (paras->NbPara >= sizeof(paras->params)/sizeof(paras->params[0]))
		return veFalse;
	paras->params[paras->NbPara++] = str;
	return veTrue;
}

static void parseParamsCleanup(char const * str) {}

static void ve_atFreeCmdPreParser(adl_atCmdPreParser_t* paras)
{
	if (paras)
		ve_free(paras->ParaList);
	ve_free(paras);
}
#endif

s8 parseParams(adl_atCmdPreParser_t *paras, char const * str)
{
	char *p;
	char *begin = NULL;
	char *tmp;
	veBool inStr = veFalse;
	s8 ret = OK;

	if (!(tmp = parseParamsInit(paras, str)))
		return ERROR;

	begin = p = tmp;
	for(;;) {
		char c;
		switch ((c = *p))
		{
		case '"':
			if (!inStr)
			{
				begin = p + 1;
				inStr = veTrue;
				break;
			}
			inStr = veFalse;
			// fall through
		case ',':
		case 0:
			if (begin != p) {
				*p = 0;
				if (!parseParamsAdd(paras, begin)) {
					ret = ERROR;
					goto cleanup;
				}
				ve_qtrace("%d: '%s'", paras->NbPara, begin);
			}

			begin = p + 1;
			if (c == 0) {
				ret = (inStr ? ERROR : OK);
				goto cleanup;
			}
			break;

		case '\\':
			if (inStr && p[1] == '"')
				p++;
			break;

		case ' ':
			if (!inStr && begin == p)
				begin = p + 1;
			break;

		default:
			break;
		}
		p++;
	}

cleanup:
	parseParamsCleanup(tmp);
	return ret;
}

/**
 * @brief Parses an AT string to a adl_atCmdPreParser_t.
 *
 * @note
 * 	it must not already contain an argument list. The argument list is
 *  automatically freed when the atCmdPreParser is freed @see
 *  ve_atFreeCmdPreParser.
 *
 */
s8 ve_atCmdParse(adl_atCmdPreParser_t *paras, char const *atcmd)
{
	size_t atCmdLength;

	ve_assert(paras->ParaList == NULL && paras->NbPara == 0);

	// figure out the type of command,
	atCmdLength = strlen(atcmd); // length of the AT command itself

	// AT+VCMD => act
	if (atCmdLength >= paras->StrLength)
	{
		paras->Type = ADL_CMD_TYPE_ACT;
		return OK;
	}

	switch (paras->StrData[atCmdLength])
	{
	case '=':
		// AT+VCMD=?				=> test
		if ( atCmdLength+1 <= paras->StrLength && paras->StrData[atCmdLength+1] == '?')
		{
			paras->Type = ADL_CMD_TYPE_TEST;
			return OK;
		}
		// AT+VCMD="something"		=> para
		paras->Type = ADL_CMD_TYPE_PARA;
		return parseParams(paras, &paras->StrData[atCmdLength+1]);

	case '?':
		// AT+VCMD?					=> read
		paras->Type = ADL_CMD_TYPE_READ;
		break;
	default:
		return ERROR;
	}

	return OK;
}

/// @}
