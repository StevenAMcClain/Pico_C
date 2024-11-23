// File: Queue.c

#include "Common.h"

#include "Queue.h"

#include <stdlib.h>
#include <string.h>


PUBLIC void Queue_Clear(QUEUE* q)           // Clear all items from queue.
{
	q->getp = q->putp = q->buffer;
	q->count = 0;
}


PUBLIC QUEUE* Queue_Initialize(size_t size)         // Allocate a new queue.
{
	QUEUE *q = malloc(sizeof(QUEUE) + size);

	if (q)
	{
		q->size = size;
		q->buffer = AFTER_PTR(q, sizeof(QUEUE));
		q->bufend = q->buffer + size;
		Queue_Clear(q);
	}
	return q;
}


PUBLIC void DeQue_Terminate(QUEUE* q)         // Release queue.
{
	if (q)
	{
		memset(q, 0, sizeof(QUEUE) + q->size);
		free(q);
	}
}


PRIVATE void* next_ptr(QUEUE*q, uint32_t* ptr)
{
	if (++ptr >= q->bufend)
	{
		ptr = q->buffer;
	}
	return ptr;
}


PRIVATE void* prev_ptr(QUEUE*q, uint32_t* ptr)
{
	if (--ptr < q->buffer)
	{
		ptr = q->bufend - 1;
	}
	return ptr;
}


PUBLIC void Queue_Push(QUEUE* q, uint32_t val)  // Push to fifo (front).
{
	// if (q && q->count < q->size)
	// {
	// 	++q->count;
	// 	*(uint32_t*)(q->putp) = (uint32_t)val;
	// 	q->putp = next_ptr(q, q->putp);
	// }
}


PUBLIC void Queue_Add(QUEUE* q, uint32_t val)   // Add to end of queue.
{
	if (q && q->count < q->size)
	{
		++q->count;
		*q->putp = val;
		q->putp = next_ptr(q, q->putp);
	}
}


PUBLIC uint32_t Queue_Pop(QUEUE* q)             // Get next item from front of queue.
{
	uint32_t val = 0;

	if (q && q->count)
	{
		val = *q->getp;
		q->getp = next_ptr(q, q->getp);
		--q->count;
	}
	return val;
}


// EndFile: Queue.c
