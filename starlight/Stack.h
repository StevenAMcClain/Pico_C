// File: Stack.h

#include "pico/mutex.h"

typedef struct Stack
{
    mutex_t mutex;

	size_t size;		// Maximum number of entries in queue.
	size_t count;		// Number currently in the queue.

	uint32_t* top;		// Points to next place to add (end of queue).

	uint32_t* buffer;	// Points to buffer for queue.

} STACK;


extern STACK*   Stack_Initialize(size_t size);          // Allocate a new queue.
extern void     Stack_Terminate(STACK* q);              // Release queue.
extern void     Stack_Clear(STACK* q);                  // Clear all items from queue.

extern void     Stack_Push(STACK* q, uint32_t val);     // Add to end of queue.
extern uint32_t Stack_Pop(STACK* q);                    // Get next item from queue.


// EndFile: Stack.h
