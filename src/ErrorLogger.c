/* ErrorLogger.c
 *
 * This file logs the unexpected events that occur within the application
 * while handling exception logging.
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

#include "TextFileHeader.h"
#include "globalHeader.h"

static FILE * File_p;
static EVENT_PROCESSOR_STRUCTURE * EventProcessor;
static char tempBuf[MAX_WRITE_BUFFER_LENGTH];

static int openFileForWriting(const char * fileName)
{
	return (NULL == (File_p = fopen(fileName, WRITE_MODE)) ? FAILURE : SUCCESS);
}

static int writeToFile(const char * str)
{
	return (fprintf(File_p, "%s", str) > 0) ? SUCCESS : FAILURE;
}

static void closeFile(void)
{
	fclose(File_p);
	printf("Closed Error Log File\n");
}

static void * logInThread(void * msg)
{
	memset(tempBuf, 0, sizeof(tempBuf));
	sprintf(tempBuf, "%s\n", (char *) msg);
	if (writeToFile(tempBuf) != SUCCESS)
		printf("Unable to Log Error: %s\n", tempBuf);
	return NULL;
}

void initializeErrorLogger(const char * fileName)
{
	printf("%s\n",
			(openFileForWriting(fileName) == SUCCESS ? "Error File created."
					: "Failed to Open Error File."));
	EventProcessor = createEventProcessor(logInThread,
			(CLONE_FUNC_POINTER) cloneString, (FREE_FUNC_POINTER) freeString);
}

void logErrorMessage(const char * msg)
{
	addEventsForProcessing(EventProcessor, (void *) msg);
}

void shutdownErrorLogger(void)
{
	releaseEventProcessor(EventProcessor);
	closeFile();
}

