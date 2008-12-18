/* AsyncEventProcessor.c
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

#include "globalHeader.h"
#include "QueueThreadHeader.h"

static void * safeDeQueue(EVENT_PROCESSOR_STRUCTURE * eventProcessor)
{
	void * item = NULL;
	pthread_mutex_lock(&(eventProcessor->eventMutex));
	if (!deQueue(eventProcessor->argsQueue, &item))
	{
		if (!(eventProcessor->terminated))
			pthread_cond_wait(&(eventProcessor->condVar),
					&(eventProcessor->eventMutex));
		deQueue(eventProcessor->argsQueue, &item);
	}
	pthread_mutex_unlock(&(eventProcessor->eventMutex));
	return item;
}

static void * processElementsInQueue(void * args)
{
	EVENT_PROCESSOR_STRUCTURE * eventProcessor = args;
	while (TRUE)
	{
		void * item = NULL;
		while ((item = safeDeQueue(eventProcessor)) != NULL)
		{
			eventProcessor->function(item);
			eventProcessor->freeArgs(item);
			item = NULL;
		}
		if (eventProcessor->terminated)
		{
			pthread_exit(NULL);
		}
	}
	return NULL;
}

EVENT_PROCESSOR_STRUCTURE * createEventProcessor(EVENT_FUNC_POINTER function,
		CLONE_FUNC_POINTER cloneArgs, FREE_FUNC_POINTER freeArgs)
{
	EVENT_PROCESSOR_STRUCTURE * eventProcessor = malloc(
			sizeof(EVENT_PROCESSOR_STRUCTURE));
	if (eventProcessor == NULL)
	{
		logError("Cannot allocate memory for EventProcessor!");
		pthread_exit(NULL);
	}

	eventProcessor->function = function;
	eventProcessor->argsQueue = malloc(sizeof(QUEUE_STRUCTURE));
	eventProcessor->cloneArgs = cloneArgs;
	eventProcessor->freeArgs = freeArgs;
	if (eventProcessor == NULL)
	{
		logError("Cannot allocate memory for EventProcessingQueue!");
		pthread_exit(NULL);
	}

	if (initializeQueue(eventProcessor->argsQueue) == FALSE)
	{
		logError("Cannot initialize Queue!");
		pthread_exit(NULL);
	}
	eventProcessor->terminated = FALSE;
	pthread_mutex_init(&(eventProcessor->eventMutex), NULL);
	pthread_cond_init(&(eventProcessor->condVar), NULL);
	createThread(&(eventProcessor->thread), processElementsInQueue,
			(void *) eventProcessor, PTHREAD_CREATE_JOINABLE);
	return eventProcessor;
}

void addEventsForProcessing(EVENT_PROCESSOR_STRUCTURE * eventProcessor,
		void * args)
{
	void * clonedArgs = NULL;
	pthread_mutex_lock(&(eventProcessor->eventMutex));
	if (!eventProcessor->cloneArgs(&clonedArgs, args) || !enQueue(
			eventProcessor->argsQueue, clonedArgs))
	{
		logError("Cannot EnQueue!");
		eventProcessor->freeArgs(clonedArgs);
		pthread_cond_signal(&(eventProcessor->condVar));
		pthread_mutex_unlock(&(eventProcessor->eventMutex));
		releaseEventProcessor(eventProcessor);
		exit(-1);
	}
	pthread_cond_signal(&(eventProcessor->condVar));
	pthread_mutex_unlock(&(eventProcessor->eventMutex));
}

void releaseEventProcessor(EVENT_PROCESSOR_STRUCTURE * eventProcessor)
{
	printf("Logging Sent. Setting terminated TRUE.\n");
	eventProcessor->terminated = TRUE;
	pthread_cond_signal(&(eventProcessor->condVar));
	pthread_join(eventProcessor->thread, NULL);
	/* This call may replace the above one - Test and see.
	 * pthread_exit(NULL);
	 */
	printf("Thread Terminated.\n");
	pthread_mutex_destroy(&(eventProcessor->eventMutex));
	pthread_cond_destroy(&(eventProcessor->condVar));
	finaliseQueue(eventProcessor->argsQueue);
	free(eventProcessor->argsQueue);
	free(eventProcessor);
}
