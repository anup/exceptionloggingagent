/* TextFileLogger.c
 *
 * This file logs the captured Exception Info in a Text File through the
 * Asynchronous Event Processor.
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

#include "globalHeader.h"
#include "TextFileHeader.h"

static FILE * File_p;
static EVENT_PROCESSOR_STRUCTURE * EventProcessor;

static char tempBuf[MAX_WRITE_BUFFER_LENGTH];

static int openFileForWriting(const char * fileName)
{
	return (NULL == (File_p = fopen(fileName, WRITE_MODE)) ? FAILURE : SUCCESS);
}

static int writeToFile(const char * str)
{
	//printf("\nData to dump In File: %s", str);
	return (fprintf(File_p, "%s", str) > 0) ? SUCCESS : FAILURE;
}

static void closeFile()
{
	fclose(File_p);
	printf("Closed File\n");
}

static int dumpStackData(STACKTRACE_STRUCTURE * stack_p, int stackCount)
{
	int i;
	int result = SUCCESS;

	for (i = 0; i < stackCount; i++)
	{
		memset(tempBuf, 0, sizeof(tempBuf));
		sprintf(tempBuf, "%s%s%s%s%s%s%s%d", FIELD_DELIMITER, getString(
				stack_p[i].methodName), FIELD_DELIMITER, getString(
				stack_p[i].methodSig), FIELD_DELIMITER, getString(
				stack_p[i].className), FIELD_DELIMITER, stack_p[i].lineNumber);

		result = result && writeToFile(tempBuf);
		if (result == FAILURE)
			break;
	}
	return result;
}

static int dumpMasterData(EXCEPTION_LOG_STRUCTURE *excp_p)
{
	char * timeStamp = asctime(&(excp_p->timeStamp));
	if (timeStamp != NULL)
		timeStamp[strlen(timeStamp) - 1] = '\0';

	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%d%s%d", getString(
			timeStamp), FIELD_DELIMITER, getString(excp_p->msg),
			FIELD_DELIMITER, getString(excp_p->class), FIELD_DELIMITER,
			getString(excp_p->sourceFileName), FIELD_DELIMITER, getString(
					excp_p->threadName), FIELD_DELIMITER, getString(
					excp_p->catchMethodname), FIELD_DELIMITER, getString(
					excp_p->catchMethodSig), FIELD_DELIMITER, getString(
					excp_p->catchClassName), FIELD_DELIMITER,
			excp_p->catchMethodLineNumber, FIELD_DELIMITER,
			excp_p->frameCount);
	return (writeToFile(tempBuf));
}

static void * logInThread(void * msg)
{
	EXCEPTION_LOG_STRUCTURE * excp_p = (EXCEPTION_LOG_STRUCTURE *) msg;

	if (dumpMasterData(excp_p) != SUCCESS)
	{
		logError("MasterData Could not be dumped in TextFile.");
	}
	else
	{
		if (excp_p->stackTrace != NULL)
		{
			if (dumpStackData(excp_p->stackTrace, excp_p->frameCount)
					!= SUCCESS)
				logError("StackData Could not be dumped in TextFile.");
		}
	}
	return NULL;
}

void initializeLogger(const char * fileName, DEALLOCATE_FUNC_POINTER func_p)
{
	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "%s",
			(openFileForWriting(fileName) == SUCCESS ? "Log File created."
					: "Failed to Open File."));
	logError(tempBuf);
	EventProcessor = createEventProcessor(logInThread, cloneExceptionPointer,
			func_p);
}

void logMessage(const EXCEPTION_LOG_STRUCTURE * msg)
{
	addEventsForProcessing(EventProcessor, (void *) msg);
}

void shutdownLogger()
{
	releaseEventProcessor(EventProcessor);
	closeFile();
}
