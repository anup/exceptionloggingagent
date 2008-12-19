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
static MYSQL_BIND bindMasterData[11];

static struct MySQLMaster MySQLMasterData;
static char StackInfo[MYSQL_BIND_BUFFER_LENGTH];

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

	bindMasterData[10].buffer_type = MYSQL_TYPE_STRING;
	bindMasterData[10].buffer = (char *) &(MySQLMasterData.StackInfo);
	bindMasterData[10].is_null = 0;
	bindMasterData[10].length = &(MySQLMasterData.lenSI);
	bindMasterData[10].buffer_length = MYSQL_BIND_BUFFER_LENGTH;

	return (mysql_stmt_bind_param(MasterLogStmt, bindMasterData) ? FAILURE
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
	result = result && bindMasterQueryParameters();
	return result;
}

static int exceuteQuery(MYSQL_STMT * stmt)
{
	return (mysql_stmt_execute(stmt)) ? FAILURE : SUCCESS;
}

static char * getStackData(STACKTRACE_STRUCTURE * stack_p, int stackCount,
		unsigned long * stackSize)
{
	int i;
	char tempBuf[MYSQL_BIND_BUFFER_LENGTH];
	memset(StackInfo, 0, sizeof(StackInfo));
	for (i = 0; i < stackCount; i++)
	{
		memset(tempBuf, 0, sizeof(tempBuf));
		sprintf(tempBuf, "%s%s%s%s%s%s%s%d\n", FIELD_DELIMITER, getString(
				stack_p[i].methodName), FIELD_DELIMITER, getString(
				stack_p[i].methodSig), FIELD_DELIMITER, getString(
				stack_p[i].className), FIELD_DELIMITER, stack_p[i].lineNumber);
		strcat(StackInfo, tempBuf);
	}
	*stackSize = strlen(StackInfo);
	return StackInfo;
}

static int dumpExceptionData(EXCEPTION_LOG_STRUCTURE *excp_p)
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

	strcpy(MySQLMasterData.StackInfo, getString(getStackData(
			excp_p->stackTrace, excp_p->frameCount, &MySQLMasterData.lenSI)));

	return (exceuteQuery(MasterLogStmt));
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
	memset(ErrorStr, 0, sizeof(ErrorStr));

	if (dumpExceptionData(excp_p) != SUCCESS)
	{
		sprintf(ErrorStr, "MasterData Could not be dumped in db: Error %s",
				getString((char *) mysql_error(Conn)));
		logError(ErrorStr);
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
