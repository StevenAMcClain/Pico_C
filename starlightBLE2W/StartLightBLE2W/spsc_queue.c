// File: spsc_queue.c

#include "common.h"
#include "spsc_queue.h"

#include <hardware/sync.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define BUF_COUNT 32   // BUF_COUNT must be power of 2.
#define BUF_SIZE  256

PRIVATE char buf_pool[BUF_COUNT][BUF_SIZE];
PRIVATE uint16_t buf_len[BUF_COUNT];

_Static_assert((BUF_COUNT & (BUF_COUNT - 1)) == 0,
               "BUF_COUNT must be a power of 2");

typedef struct 
{
    uint8_t items[BUF_COUNT];

    volatile uint32_t head;    // producer writes
    volatile uint32_t tail;    // consumer reads

} spsc_t;

PRIVATE spsc_t free_list;       // free list of buffer indices

PRIVATE spsc_t queue;    // Producer: core0, Consumer: core1


PRIVATE void init_free_list(void)           // initialize free list of buffer indices
{
    _Static_assert((BUF_COUNT & (BUF_COUNT - 1)) == 0, "BUF_COUNT must be power of 2");

    free_list.head = 0;
    free_list.tail = 0;

    for (uint8_t i = 0; i < (BUF_COUNT - 1); i++) 
    {
        free_list.items[i] = i;
        free_list.head = (free_list.head + 1) & (BUF_COUNT - 1);
    }
}

// Pop from free list (allocate buffer)
//
PRIVATE inline bool free_pop(uint8_t *idx)          // returns true if an item was popped, false if queue is empty
{
    if (free_list.tail == free_list.head)
        return false; // empty

    *idx = free_list.items[free_list.tail];
    __dmb();
    free_list.tail = (free_list.tail + 1) & (BUF_COUNT - 1);

    return true;
}

// Push to free list (release buffer)
//
PRIVATE inline bool free_push(uint8_t idx)              // returns true if an item was pushed, false if queue is full
{
    uint32_t next = (free_list.head + 1) & (BUF_COUNT - 1);

    if (next == free_list.tail)
        return false; // full (should never happen)

    free_list.items[free_list.head] = idx;
    __dmb();
    free_list.head = next;

    return true;
}

// allocator
//
PRIVATE uint8_t next_free_buffer(void)          // returns index of next free buffer, blocks if none available
{
    uint8_t idx;

    while (!free_pop(&idx)) {
        __wfe();   // wait until a buffer is released
    }

    return idx;
}

// return buffer to pool

PRIVATE void release_buffer(uint8_t idx)    // mark buffer as free
{
    __sev();        // wake waiting core

    while (!free_push(idx)) 
    {
        tight_loop_contents();   // should never block
    }
}

// Producer
//
PRIVATE bool spsc_push(spsc_t *q, uint8_t idx)    // returns true if an item was pushed, false if queue is full
{
    uint32_t next = (q->head + 1) & (BUF_COUNT - 1);

    if (next == q->tail)
        return false; // full

    q->items[q->head] = idx;
    __dmb();
    q->head = next;
    return true;
}

// Consumer
//
PRIVATE bool spsc_pop(spsc_t *q, uint8_t *idx)    // returns true if an item was popped, false if queue is empty
{
    if (q->tail == q->head)
        return false; // empty

    *idx = q->items[q->tail];
    __dmb();
    q->tail = (q->tail + 1) & (BUF_COUNT - 1);
    return true;
}


PUBLIC void ble_rx(const char* data, size_t len)   // called from BLE thread
{
    size_t offset = 0;

    while (offset < len)
    {
        size_t chunk = len - offset;
        if (chunk > BUF_SIZE)
            chunk = BUF_SIZE;

        uint8_t idx = next_free_buffer();   // allocate buffer

        memcpy(buf_pool[idx], data + offset, chunk);
        buf_len[idx] = chunk;

        // push into queue (block until space)
        while (!spsc_push(&queue, idx))                             // Should have timeout....
            __wfe();

        offset += chunk;
    }
}


PUBLIC void consume(packet_processor_t processor)   // called from core1 thread
{
    uint8_t idx;    // index of buffer in pool

    if (spsc_pop(&queue, &idx))              // Pop the next buffer index from the queue
    {
        __sev();                                        // Signal that a buffer has been consumed (if needed)
        processor(buf_pool[idx], buf_len[idx]);         // Call the provided processor function with the data and length
        release_buffer(idx); // mark free               //
    }
}


PUBLIC void spsc_queue_init(void)
{
    queue.head = queue.tail = 0;

    init_free_list();    
}


// End of file: spsc_queue.c
