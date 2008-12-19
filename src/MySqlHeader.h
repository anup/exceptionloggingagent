/* MySQLHeader.h
 *
 * Include file for Logging Exception into the MySql Database. Defines the
 * Logging Structure and the Queries.
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

#ifndef MYSQLHEADER_H_
#define MYSQLHEADER_H_

#ifdef WINDOWS
#include <windows.h>
#endif

#include <mysql.h>

#include "header.h"
#include "QueueThreadHeader.h"

#define MAX_DB_ERROR_MESSAGE_LENGTH 65535

#define MYSQL_BIND_BUFFER_LENGTH 65535

#define INSERT_MASTER_LOG_QUERY "insert into tbl_ExceptionLog (TimeStamp, ExceptionMsg, ExceptionType, SourceFileName, \
		ThreadName, CatchMethodName, CatchMethodSignature, CatchClassName, CatchLineNumber, StackFrameCount, Stack) \
		values (?,?,?,?,?,?,?,?,?,?,?)"

#define FIELD_DELIMITER "~"

struct MySQLMaster
{
	MYSQL_TIME ts;
	char ExceptionMsg[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenMsg;
	char ExceptionType[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenType;
	char SourceFileName[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenSFN;
	char ThreadName[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenTN;
	char CatchMethodName[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenCMN;
	char CatchMethodSignature[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenCMSig;
	char CatchClassName[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenCCN;
	char StackInfo[MYSQL_BIND_BUFFER_LENGTH];
	unsigned long lenSI;
	int ln;
	int sc;
};

#endif

/* MYSQLHEADER_H_ */
