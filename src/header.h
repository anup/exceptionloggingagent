/* header.h
 *
 * Include file for defining the Exception Structure and exposing all the common
 * function to be used by the JVMTIAgent.c
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

#ifndef HEADER_H_
#define HEADER_H_

#include <jvmti.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#define FAILURE 0
#define SUCCESS !FAILURE

typedef void (* DEALLOCATE_FUNC_POINTER)(void *);

typedef struct
{
	char * methodName;
	char * methodSig;
	char * className;
	int lineNumber;
} STACKTRACE_STRUCTURE;

typedef struct
{
	struct tm timeStamp;
	char * msg;
	char * class;
	char * threadName;
	char * sourceFileName;
	char * catchMethodname;
	char * catchMethodSig;
	char * catchClassName;
	int catchMethodLineNumber;
	int frameCount;
	STACKTRACE_STRUCTURE * stackTrace;
} EXCEPTION_LOG_STRUCTURE;

extern int initialize(const char * propFileName);
extern void shutdownConfig(void);
extern char * getTextLogFileName(void);
extern char * getErrorFileName(void);
extern char * getDBServerName(void);
extern char * getDBUserName(void);
extern char * getDBPassword(void);
extern char * getDBName(void);
extern int getTextLogging(void);
extern int getDBLogging(void);
extern int getStackFrameCount(void);

extern void initializeErrorLogger(const char * fileName);
extern void logErrorMessage(const char * msg);
extern void shutdownErrorLogger(void);

extern void initializeLogger(const char *, DEALLOCATE_FUNC_POINTER);
extern void logMessage(const EXCEPTION_LOG_STRUCTURE * msg);
extern void shutdownLogger(void);

extern void initializeDBLogger(const char *, const char *, const char *,
		const char *, DEALLOCATE_FUNC_POINTER);
extern void logMessageInDB(const EXCEPTION_LOG_STRUCTURE * msg);
extern void shutdownLoggerInDB(void);

#endif /* HEADER_H_ */

