/* MySqlLogger.c
 *
 * This file logs the captured Exception Info in a MySql DB through the
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
#include "MySqlHeader.h"

static char ErrorStr[MAX_DB_ERROR_MESSAGE_LENGTH];

static MYSQL * Conn;

static MYSQL_STMT * MasterLogStmt;
static MYSQL_STMT * StackLogStmt;
static MYSQL_BIND bindMasterData[10];
static MYSQL_BIND bindStackData[6];

static struct MySQLMaster MySQLMasterData;
static struct MySQLStack MySQLStackData;

static EVENT_PROCESSOR_STRUCTURE * EventProcessor;

static int bindMasterQueryParameters(void)
{
	memset(bindMasterData, 0, sizeof(bindMasterData));

	bindMasterData[0].buffer_type = MYSQL_TYPE_DATETIME;
	bindMasterData[0].buffer = (char *) &(MySQLMasterData.ts);
	bindMasterData[0].is_null = 0;
	bindMasterData[0].length = 0;

	bindMasterData[1].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[1].buffer = (char *) MySQLMasterData.ExceptionMsg;
	bindMasterData[1].is_null = 0;
	bindMasterData[1].length = &(MySQLMasterData.lenMsg);
	bindMasterData[1].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[2].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[2].buffer = (char *) MySQLMasterData.ExceptionType;
	bindMasterData[2].is_null = 0;
	bindMasterData[2].length = &(MySQLMasterData.lenType);
	bindMasterData[2].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[3].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[3].buffer = (char *) MySQLMasterData.SourceFileName;
	bindMasterData[3].is_null = 0;
	bindMasterData[3].length = &(MySQLMasterData.lenSFN);
	bindMasterData[3].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[4].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[4].buffer = (char *) MySQLMasterData.ThreadName;
	bindMasterData[4].is_null = 0;
	bindMasterData[4].length = &(MySQLMasterData.lenTN);
	bindMasterData[4].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[5].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[5].buffer = (char *) MySQLMasterData.CatchMethodName;
	bindMasterData[5].is_null = 0;
	bindMasterData[5].length = &(MySQLMasterData.lenCMN);
	bindMasterData[5].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[6].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[6].buffer = (char *) MySQLMasterData.CatchMethodSignature;
	bindMasterData[6].is_null = 0;
	bindMasterData[6].length = &(MySQLMasterData.lenCMSig);
	bindMasterData[6].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[7].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[7].buffer = (char *) MySQLMasterData.CatchClassName;
	bindMasterData[7].is_null = 0;
	bindMasterData[7].length = &(MySQLMasterData.lenCCN);
	bindMasterData[7].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindMasterData[8].buffer_type = MYSQL_TYPE_LONG;
	bindMasterData[8].buffer = (char *) &(MySQLMasterData.ln);
	bindMasterData[8].is_null = 0;
	bindMasterData[8].length = 0;

	bindMasterData[9].buffer_type = MYSQL_TYPE_LONG;
	bindMasterData[9].buffer = (char *) &(MySQLMasterData.sc);
	bindMasterData[9].is_null = 0;
	bindMasterData[9].length = 0;

	return (mysql_stmt_bind_param(MasterLogStmt, bindMasterData) ? FAILURE
			: SUCCESS);
}

static int bindStackQueryParameters(void)
{
	memset(bindStackData, 0, sizeof(bindStackData));

	bindStackData[0].buffer_type = MYSQL_TYPE_LONG;
	bindStackData[0].buffer = (char *) &(MySQLStackData.ExceptionId);
	bindStackData[0].is_null = 0;
	bindStackData[0].length = 0;

	bindStackData[1].buffer_type = MYSQL_TYPE_LONG;
	bindStackData[1].buffer = (char *) &(MySQLStackData.FrameId);
	bindStackData[1].is_null = 0;
	bindStackData[1].length = 0;

	bindStackData[2].buffer_type = MYSQL_TYPE_STRING;
	bindStackData[2].buffer = (char *) MySQLStackData.MethodName;
	bindStackData[2].is_null = 0;
	bindStackData[2].length = &(MySQLStackData.lenMN);
	bindStackData[2].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindStackData[3].buffer_type = MYSQL_TYPE_STRING;
	bindStackData[3].buffer = (char *) MySQLStackData.MethodSignature;
	bindStackData[3].is_null = 0;
	bindStackData[3].length = &(MySQLStackData.lenMSig);
	bindStackData[2].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindStackData[4].buffer_type = MYSQL_TYPE_STRING;
	bindStackData[4].buffer = (char *) MySQLStackData.ClassName;
	bindStackData[4].is_null = 0;
	bindStackData[4].length = &(MySQLStackData.lenCN);
	bindStackData[2].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	bindStackData[5].buffer_type = MYSQL_TYPE_LONG;
	bindStackData[5].buffer = (char *) &(MySQLStackData.LineNumber);
	bindStackData[5].is_null = 0;
	bindStackData[5].length = 0;

	return (mysql_stmt_bind_param(StackLogStmt, bindStackData) ? FAILURE
			: SUCCESS);
}

static int createDatabaseConnection(const char * server, const char *user,
		const char * password, const char * database)
{
	Conn = mysql_init(NULL);
	return (!(mysql_real_connect(Conn, server, user, password, database, 0,
			NULL, 0)) ? FAILURE : SUCCESS);
}

static int prepareQueries(void)
{
	int result = SUCCESS;
	if (Conn != NULL)
	{
		MasterLogStmt = mysql_stmt_init(Conn);
		StackLogStmt = mysql_stmt_init(Conn);
	}
	result = result && (MasterLogStmt) && (StackLogStmt);
	result = result && !mysql_stmt_prepare(MasterLogStmt,
			INSERT_MASTER_LOG_QUERY, strlen(INSERT_MASTER_LOG_QUERY) + 1);
	result = result && !mysql_stmt_prepare(StackLogStmt,
			INSERT_STACK_LOG_QUERY, strlen(INSERT_STACK_LOG_QUERY) + 1);
	result = result && bindMasterQueryParameters()
			&& bindStackQueryParameters();
	return result;
}

static int exceuteQuery(MYSQL_STMT * stmt)
{
	return (mysql_stmt_execute(stmt)) ? FAILURE : SUCCESS;
}

static int dumpMasterData(EXCEPTION_LOG_STRUCTURE *excp_p)
{
	memset(&MySQLMasterData, 0, sizeof(MySQLMasterData));

	MySQLMasterData.ts.day = excp_p->timeStamp.tm_mday;
	MySQLMasterData.ts.hour = excp_p->timeStamp.tm_hour;
	MySQLMasterData.ts.minute = excp_p->timeStamp.tm_min;
	MySQLMasterData.ts.month = excp_p->timeStamp.tm_mon;
	MySQLMasterData.ts.second = excp_p->timeStamp.tm_sec;
	MySQLMasterData.ts.year = excp_p->timeStamp.tm_year + 1900;
	MySQLMasterData.ts.time_type = MYSQL_TIMESTAMP_DATETIME;

	strcpy(MySQLMasterData.ExceptionMsg, getString(excp_p->msg));
	MySQLMasterData.lenMsg = (excp_p->msg == NULL) ? 0 : strlen(excp_p->msg)
			+ 1;

	strcpy(MySQLMasterData.ExceptionType, getString(excp_p->class));
	MySQLMasterData.lenType = (excp_p->class == NULL) ? 0 : strlen(
			excp_p->class) + 1;

	strcpy(MySQLMasterData.SourceFileName, getString(excp_p->sourceFileName));
	MySQLMasterData.lenSFN = (excp_p->sourceFileName == NULL) ? 0 : strlen(
			excp_p->sourceFileName) + 1;

	strcpy(MySQLMasterData.ThreadName, getString(excp_p->threadName));
	MySQLMasterData.lenTN = (excp_p->threadName == NULL) ? 0 : strlen(
			excp_p->threadName) + 1;

	strcpy(MySQLMasterData.CatchMethodName, getString(excp_p->catchMethodname));
	MySQLMasterData.lenCMN = (excp_p->catchMethodname == NULL) ? 0 : strlen(
			excp_p->catchMethodname) + 1;

	strcpy(MySQLMasterData.CatchMethodSignature, getString(
			excp_p->catchMethodSig));
	MySQLMasterData.lenCMSig = (excp_p->catchMethodSig == NULL) ? 0 : strlen(
			excp_p->catchMethodSig) + 1;

	strcpy(MySQLMasterData.CatchClassName, getString(excp_p->catchClassName));
	MySQLMasterData.lenCCN = (excp_p->catchClassName == NULL) ? 0 : strlen(
			excp_p->catchClassName) + 1;

	MySQLMasterData.ln = excp_p->catchMethodLineNumber;

	MySQLMasterData.sc = excp_p->frameCount;

	return (exceuteQuery(MasterLogStmt));
}

static int dumpStackData(STACKTRACE_STRUCTURE * stack_p, int stackCount,
		long exceptionId)
{
	int i;
	int result = SUCCESS;

	for (i = 0; i < stackCount; i++)
	{
		memset(&MySQLStackData, 0, sizeof(MySQLStackData));

		MySQLStackData.ExceptionId = exceptionId;

		MySQLStackData.FrameId = i + 1;

		strcpy(MySQLStackData.MethodName, getString(stack_p[i].methodName));
		MySQLStackData.lenMN = (stack_p[i].methodName == NULL) ? 0 : strlen(
				stack_p[i].methodName) + 1;

		strcpy(MySQLStackData.MethodSignature, getString(stack_p[i].methodSig));
		MySQLStackData.lenMSig = (stack_p[i].methodSig == NULL) ? 0 : strlen(
				stack_p[i].methodSig) + 1;

		strcpy(MySQLStackData.ClassName, getString(stack_p[i].className));
		MySQLStackData.lenCN = (stack_p[i].className == NULL) ? 0 : strlen(
				stack_p[i].className) + 1;

		MySQLStackData.LineNumber = stack_p[i].lineNumber;

		result = result && (exceuteQuery(StackLogStmt) ? SUCCESS : FAILURE);
		if (result == FAILURE)
			break;
	}
	return result;
}

static void closeDBConnection()
{
	mysql_stmt_close(MasterLogStmt);
	mysql_stmt_close(StackLogStmt);
	mysql_close(Conn);
	printf("Closed DB Connection\n");
}

static void * logInThread(void * msg)
{
	EXCEPTION_LOG_STRUCTURE * excp_p = (EXCEPTION_LOG_STRUCTURE *) msg;
	long exceptionId;
	memset(ErrorStr, 0, sizeof(ErrorStr));

	if (dumpMasterData(excp_p) != SUCCESS)
	{
		sprintf(ErrorStr, "MasterData Could not be dumped in db: Error %s",
				getString((char *) mysql_error(Conn)));
		logError(ErrorStr);
	}
	else
	{
		if (excp_p->stackTrace != NULL)
		{
			exceptionId = mysql_insert_id(Conn);
			if (exceptionId != 0)
			{
				if (dumpStackData(excp_p->stackTrace, excp_p->frameCount,
						exceptionId) != SUCCESS)
				{
					sprintf(ErrorStr,
							"StackData Could not be dumped in db: Error %s",
							getString((char *) mysql_error(Conn)));
					logError(ErrorStr);
				}
			}
			else
			{
				sprintf(
						ErrorStr,
						"StackData Could not be dumped in db. ExceptionId returned zero for Inserted Master Data");
				logError(ErrorStr);
			}

		}
	}
	return NULL;
}

void initializeDBLogger(const char * server, const char *user,
		const char * password, const char * database,
		DEALLOCATE_FUNC_POINTER func_p)
{
	EventProcessor = createEventProcessor(logInThread, cloneExceptionPointer,
			func_p);

	memset(ErrorStr, 0, sizeof(ErrorStr));

	if (createDatabaseConnection(server, user, password, database) == SUCCESS)
	{
		sprintf(ErrorStr, "DB connection created.");
	}
	else
	{
		sprintf(ErrorStr, "Failed to Create DB Connection. Error: %s",
				getString((char *) mysql_error(Conn)));
	}
	logError(ErrorStr);

	if (prepareQueries() == SUCCESS)
	{
		sprintf(ErrorStr, "Queries Prepared Successfully");
	}
	else
	{
		sprintf(ErrorStr, "Failed to Prepare Queries. Error: %s", getString(
				(char *) mysql_error(Conn)));
	}
	logError(ErrorStr);
}

void logMessageInDB(const EXCEPTION_LOG_STRUCTURE * msg)
{
	addEventsForProcessing(EventProcessor, (void *) msg);
}

void shutdownLoggerInDB()
{
	releaseEventProcessor(EventProcessor);
	closeDBConnection();
}
