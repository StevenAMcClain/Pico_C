// File: Queue.c

#include "Common.h"

#include "Stack.h"

#include <stdlib.h>
#include <string.h>


PUBLIC void Stack_Clear(STACK* q)           // Clear all items from queue.
{
    if (q)
    {
        mutex_enter_blocking(&q->mutex);

        q->count = 0;
        q->top = q->buffer;

        mutex_exit(&q->mutex);
    }
}


PUBLIC STACK* Stack_Initialize(size_t size)         // Allocate a new queue.
{
	STACK *q = malloc(sizeof(STACK) + (sizeof(uint32_t) * size));

	if (q)
	{
        memset(q, 0, sizeof(*q));
		q->size = size;
		q->buffer = AFTER_PTR(q, sizeof(STACK));
        mutex_init(&q->mutex);
		Stack_Clear(q);
	}
	return q;
}


PUBLIC void Stack_Terminate(STACK* q)         // Release queue.
{
	if (q)
	{
		memset(q, 0, sizeof(STACK) + q->size);
		free(q);
	}
}


PUBLIC void Stack_Push(STACK* q, uint32_t val)   // Add to end of queue.
{
    mutex_enter_blocking(&q->mutex);

	if (q && q->count < q->size)
	{
		++q->count;
		*(q->top++) = val;
	}

    mutex_exit(&q->mutex);
}


PUBLIC uint32_t Stack_Pop(STACK* q)             // Get next item from front of queue.
{
	uint32_t val = 0;

    mutex_enter_blocking(&q->mutex);

	if (q && q->count)
	{
		--q->count;
		val = *(--q->top);
	}

    mutex_exit(&q->mutex);
	return val;
}



// EndFile: Stack.c
