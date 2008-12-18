/* Queue.c
 *
 * This file implements an infinitely growing Circular Queue using a Doubling
 * growth Strategy
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

static int isQueueEmty(QUEUE_STRUCTURE * Q)
{
	return (Q->head == Q->tail);
}

static int isQueueFull(QUEUE_STRUCTURE * Q)
{
	return (Q->head == (Q->tail + 1) % Q->length);
}

/*
 * Uses Doubling Strategy to grow the Queue
 */
static int growQueue(QUEUE_STRUCTURE * Q)
{
	long newSize;
	if ((Q->length * 2) > MAXINT)
	{
		logError("Cannot grow Queue. Size of Queue will cross MAXINT");
		return FALSE;
	}
	newSize = Q->length * 2;
	Q->queueArray = realloc(Q->queueArray, sizeof(QUEUE_ITEM *) * newSize);
	if (Q->queueArray == NULL)
	{
		logError("Cannot grow Queue. Not enough memory");
		return FALSE;
	}
	if (Q->head > Q->tail) /* Data from the start needs to be copied to the end in the new allocated array*/
	{
		memcpy(&(Q->queueArray[Q->length]), &(Q->queueArray[0]), (Q->tail)
				* sizeof(QUEUE_ITEM *));
		Q->tail += Q->length;
	}
	Q->length *= 2;
	return TRUE;
}

int initializeQueue(QUEUE_STRUCTURE * Q)
{
	Q->length = DEFAULT_QUEUE_LENGTH;
	Q->head = 0;
	Q->tail = 0;
	Q->queueArray = calloc(sizeof(QUEUE_ITEM *), Q->length);
	Q->itemAdded = 0;
	Q->itemDeleted = 0;
	if (Q->queueArray == NULL)
	{
		logError("Cannot allocate memory for Queue");
		return FALSE;
	}
	return TRUE;
}

void finaliseQueue(QUEUE_STRUCTURE * Q)
{
	free(Q->queueArray);
}

int enQueue(QUEUE_STRUCTURE * Q, void * item)
{
	if (isQueueFull(Q))
	{
		if (growQueue(Q) == FALSE)
			return FALSE;
	}
	QUEUE_ITEM * tempItem = malloc(sizeof(QUEUE_ITEM));
	if (tempItem == NULL)
	{
		logError("Cannot allocate memory for inserting item in Queue!");
		return FALSE;
	}

	tempItem->item = item;

	Q->queueArray[Q->tail] = tempItem;
	if (Q->tail == Q->length - 1)
	{
		Q->tail = 0;
	}
	else
	{
		Q->tail++;
	}
	Q->itemAdded++;
	return TRUE;
}

int deQueue(QUEUE_STRUCTURE * Q, void ** item)
{
	if (isQueueEmty(Q))
	{
		return FALSE;
	}
	QUEUE_ITEM * tempItem = Q->queueArray[Q->head];
	if (Q->head == Q->length - 1)
	{
		Q->head = 0;
	}
	else
	{
		Q->head++;
	}
	*item = tempItem->item;
	free(tempItem);
	Q->itemDeleted++;
	return TRUE;
}
