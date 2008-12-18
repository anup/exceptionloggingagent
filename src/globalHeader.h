/* globalHeader.h
 *
 * This file implements an Asynchronous Event Processor using a Queue and a
 * pthread implementation of a Thread.
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

#ifndef GLOBALHEADER_H_
#define GLOBALHEADER_H_

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

extern void logError(const char * msg);
extern int cloneString(void ** destStr_p, void * srcStr);
extern void freeString(void * str);
extern int cloneExceptionPointer(void ** clonedMsg, void * msg);

extern char * getString(char * str);
extern void logError(const char * msg);

extern struct tm getTimeStamp(void);

#endif /* GLOBALHEADER_H_ */
