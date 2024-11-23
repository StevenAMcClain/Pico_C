// File: DeQueu.h


typedef struct Queue
{
	size_t size;		// Maximum number of entries in queue.
	size_t count;		// Number currently in the queue.

	uint32_t* putp;		// Points to next place to add (end of queue).
	uint32_t* getp;		// Points to next place to get (front of queue).

	uint32_t* buffer;	// Points to buffer for queue.
    uint32_t* bufend;   // Points to end of buffer.

} QUEUE;


extern QUEUE* Queue_Initialize(size_t size);      // Allocate a new queue.

extern void Queue_Terminate(QUEUE* q);            // Release queue.

extern void Queue_Clear(QUEUE* q);               // Clear all items from queue.

extern void Queue_Push(QUEUE* q, uint32_t val);  // Push to fifo (front).

extern void Queue_Add(QUEUE* q, uint32_t val);   // Add to end of queue.

extern uint32_t Queue_Pop(QUEUE* q);             // Get next item from queue.


// EndFile: Queue.h
