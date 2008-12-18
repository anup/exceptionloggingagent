/* Thread.c
 *
 * This file creates a pthread and returns a handle to the same.
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
#include "QueueThreadHeader.h"

pthread_t createThread(pthread_t * Thread_P, EVENT_FUNC_POINTER function,
		void * args, int detachable)
{
	pthread_attr_t attr;
	int rc;
	char tempBuf[50];

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, detachable);

	rc = pthread_create(Thread_P, &attr, function, args);
	if (rc)
	{
		sprintf(tempBuf, "ERROR; return code from pthread_create() is %d", rc);
		logError(tempBuf);
		exit(-1);
	}

	pthread_attr_destroy(&attr);
	return (*Thread_P);
}
