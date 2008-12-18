/* QueueThreadHeader.h
 *
 * Include file for the Asynchronous Event Processor, Queue, and Thread.
 * Defines the Queue Item and the Event Processor Structure.
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

#ifndef QUEUETHREADHEADER_H_
#define QUEUETHREADHEADER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#define DEFAULT_QUEUE_LENGTH 100
#define MAXINT (pow(2, sizeof(int) * 8 - 1) -1)

typedef void * (* EVENT_FUNC_POINTER)(void *);
typedef int (* CLONE_FUNC_POINTER)(void **, void *);
typedef void (* FREE_FUNC_POINTER)(void *);

typedef struct
{
	void * item;
} QUEUE_ITEM;

typedef struct
{
	QUEUE_ITEM * *queueArray;
	int length;
	int head; /*location of the first element*/
	int tail; /*location of newly arriving element*/
	int itemAdded;
	int itemDeleted;
} QUEUE_STRUCTURE;

typedef struct
{
	EVENT_FUNC_POINTER function;
	QUEUE_STRUCTURE * argsQueue;
	CLONE_FUNC_POINTER cloneArgs;
	FREE_FUNC_POINTER freeArgs;
	pthread_mutex_t eventMutex;
	pthread_cond_t condVar;
	pthread_t thread;
	int terminated;
} EVENT_PROCESSOR_STRUCTURE;

extern int deQueue(QUEUE_STRUCTURE * Q, void ** item);
extern int enQueue(QUEUE_STRUCTURE * Q, void * item);
extern int initializeQueue(QUEUE_STRUCTURE * Q);
extern void finaliseQueue(QUEUE_STRUCTURE * Q);

pthread_t createThread(pthread_t *, EVENT_FUNC_POINTER, void *, int);

extern EVENT_PROCESSOR_STRUCTURE * createEventProcessor(
		EVENT_FUNC_POINTER function, CLONE_FUNC_POINTER cloneArgs,
		FREE_FUNC_POINTER freeArgs);
extern void addEventsForProcessing(EVENT_PROCESSOR_STRUCTURE * eventProcessor,
		void * args);
extern void releaseEventProcessor(EVENT_PROCESSOR_STRUCTURE * eventProcessor);

#endif /* QUEUETHREADHEADER_H_ */
