/* util.c
 *
 * This file implements some utility functions used by various parts of the
 * project.
 *
 * Author: <anup.k@directi.com>
 *
 * Copyright (C) 2008 Directi
 *
 * This file is part of ExceptionLoggingAgent.
 *
 * ExceptionLoggingAgent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * ExceptionLoggingAgent is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "header.h"
#include "globalHeader.h"

char * getString(char * str)
{
	return (str == NULL ? "" : str);
}

struct tm getTimeStamp()
{
	struct tm * timeStructure;
	time_t timeInMilli;
	timeInMilli = time(NULL);
	timeStructure = localtime(&timeInMilli);
	return *timeStructure;
}

void logError(const char * msg)
{
	char tempBuf[32768];
	memset(tempBuf, 0, sizeof(tempBuf));
	time_t currentTime = time(NULL);
	char * timeStamp = ctime(&currentTime);
	if (timeStamp != NULL)
		timeStamp[strlen(timeStamp) - 1] = '\0';
	sprintf(tempBuf, "%s:%s%c", getString(timeStamp), msg, '\0');
	logErrorMessage(tempBuf);
}

int cloneString(void ** dest_p, void * src)
{
	char * destStr = NULL;
	char * srcStr = src;
	if (src == NULL)
		return TRUE;
	destStr = (char *) malloc(strlen(srcStr) + 1); /* +1 for '\0' */
	if (destStr == NULL)
		return FALSE;
	strcpy(destStr, srcStr);
	*dest_p = destStr;
	return TRUE;
}

void freeString(void * str)
{
	if (str != NULL)
		free(str);
}

static void init(EXCEPTION_LOG_STRUCTURE * excp_p)
{
	excp_p->catchClassName = NULL;
	excp_p->catchMethodSig = NULL;
	excp_p->catchMethodLineNumber = -1;
	excp_p->catchMethodname = NULL;
	excp_p->class = NULL;
	excp_p->frameCount = 0;
	excp_p->msg = NULL;
	excp_p->stackTrace = NULL;
	excp_p->threadName = NULL;
	excp_p->sourceFileName = NULL;
}

int cloneExceptionPointer(void ** clonedMsg, void * msg)
{
	EXCEPTION_LOG_STRUCTURE * excp_p = (EXCEPTION_LOG_STRUCTURE *) msg;
	EXCEPTION_LOG_STRUCTURE * clonedExcp_p;
	STACKTRACE_STRUCTURE stackTrace;
	int i;

	clonedExcp_p = malloc(sizeof(EXCEPTION_LOG_STRUCTURE));
	if (clonedExcp_p == NULL)
	{
		logError("Error Cloning Exception");
		return FALSE;
	}
	init(clonedExcp_p);
	clonedExcp_p->timeStamp = excp_p->timeStamp;
	if (cloneString((void **) &(clonedExcp_p->msg), (void *) excp_p->msg)
			== FALSE)
	{
		logError("Error Cloning Exception Message");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->class), (void *) excp_p->class)
			== FALSE)
	{
		logError("Error Cloning Exception Class");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->threadName),
			(void *) excp_p->threadName) == FALSE)
	{
		logError("Error Cloning Exception ThreadName");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->sourceFileName),
			(void *) excp_p->sourceFileName) == FALSE)
	{
		logError("Error Cloning Exception SourceFileName");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->catchMethodname),
			(void *) excp_p->catchMethodname) == FALSE)
	{
		logError("Error Cloning Exception Catch Method Name");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->catchMethodSig),
			(void *) excp_p->catchMethodSig) == FALSE)
	{
		logError("Error Cloning Exception Catch Method Signature");
		return FALSE;
	}
	if (cloneString((void **) &(clonedExcp_p->catchClassName),
			(void *) excp_p->catchClassName) == FALSE)
	{
		logError("Error Cloning Exception Catch Class Name");
		return FALSE;
	}

	clonedExcp_p->catchMethodLineNumber = excp_p->catchMethodLineNumber;
	clonedExcp_p->frameCount = excp_p->frameCount;

	clonedExcp_p->stackTrace = malloc(sizeof(STACKTRACE_STRUCTURE)
			* excp_p->frameCount);
	if (clonedExcp_p->stackTrace == NULL)
	{
		logError("Error Cloning StackTrace");
		return FALSE;
	}

	for (i = 0; i < excp_p->frameCount; i++)
	{
		stackTrace = excp_p->stackTrace[i];

		clonedExcp_p->stackTrace[i].lineNumber = stackTrace.lineNumber;
		if (cloneString((void **) &(clonedExcp_p->stackTrace[i].methodName),
				(void *) stackTrace.methodName) == FALSE)
		{
			logError("Error Cloning Stack Method Name");
			return FALSE;
		}
		if (cloneString((void **) &(clonedExcp_p->stackTrace[i].methodSig),
				(void *) stackTrace.methodSig) == FALSE)
		{
			logError("Error Cloning Stack Method Sig");
			return FALSE;
		}
		if (cloneString((void **) &(clonedExcp_p->stackTrace[i].className),
				(void *) stackTrace.className) == FALSE)
		{
			logError("Error Cloning Stack Class Name");
			return FALSE;
		}
	}

	*clonedMsg = clonedExcp_p;
	return TRUE;
}
