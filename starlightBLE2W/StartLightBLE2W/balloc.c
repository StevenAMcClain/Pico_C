// File: balloc.c

#include "common.h"
#include "balloc.h"

#include <string.h>


typedef struct balloc
{
    bool reserved;      // When true all unused is reserved.

    uint8_t* base;      // Pointer to the base of the buffer
    uint8_t* next;      // Pointer to the next free memory location
    
    size_t size;        // Total size of empty buffer
    size_t room_left;   // Remaining bytes available

    size_t room_left_min;   // Minimum remaining bytes available ever seen.

} balloc_t;


PUBLIC void Balloc_Clear(balloc_h bh)
{
    if (bh)
    {
        balloc_t* bp = bh;

        DBG(0, DEBUG_BALLOC, "used: %u\n", (unsigned)(bp->next - bp->base));

        if (bp->room_left < bp->room_left_min)
        {
            bp->room_left_min = bp->room_left;
        }
        bp->next = bp->base;
        bp->room_left = bp->size;
        bp->reserved = false;
    }
}


PUBLIC balloc_h Balloc_Init(uint8_t* base, size_t size)
{
    balloc_t* bp = NULL;

    if (size > sizeof(balloc_t))
    {
        bp = (balloc_t*)base;
        bp->next = bp->base = base + sizeof(balloc_t);
        bp->room_left_min = bp->room_left = bp->size = size - sizeof(balloc_t);
        bp->reserved = false;
    }
    return (void*)bp;
}


PUBLIC void* Balloc(balloc_h bh, size_t size)
{
    if (!bh || size == 0) { return NULL; }

    balloc_t* bp = (balloc_t*)bh;

    if (bp->reserved) { return NULL; }

    const size_t ALIGN = _Alignof(max_align_t);    // Required alignment (you can change this)

    uintptr_t raw = (uintptr_t)bp->next;
    uintptr_t aligned = (raw + (ALIGN - 1)) & ~(ALIGN - 1);

    size_t padding = aligned - raw;

    if (  padding > bp->room_left || size > bp->room_left - padding) { return NULL; }        // Check total required space

    bp->next = (uint8_t*)aligned;          // Apply alignment
    bp->room_left -= padding;

    void* ptr = bp->next;                  // Allocate
    bp->next += size;
    bp->room_left -= size;

    return ptr;
}
// 

PUBLIC void* Ballocz(balloc_h bp, size_t size)
{
    uint8_t* ptr = Balloc(bp, size);

    if (ptr) { memset(ptr, 0, size); }

    return ptr;
}


PUBLIC void* Balloc_Base(balloc_h bh)
{
    return bh ? ((balloc_t*)bh)->base : NULL;
}


PUBLIC bool Balloc_Reserve(balloc_h bh)
{
    bool result = false;

    if (bh)
    {
        balloc_t* bp = bh;

        if (!bp->reserved)
        {
            const size_t ALIGN = _Alignof(max_align_t);
            uintptr_t raw     = (uintptr_t)bp->next;
            uintptr_t aligned = (raw + (ALIGN - 1)) & ~(ALIGN - 1);
            size_t    pad     = aligned - raw;

            if (pad <= bp->room_left)
            {
                bp->next       = (uint8_t*)aligned;
                bp->room_left -= pad;
                bp->reserved   = true;
                result         = true;
            }
        }
    }
    return result;
}


PUBLIC void Balloc_Release(balloc_h bh)
{
    if (bh) { balloc_t* bp = bh; bp->reserved = false; }
}


PUBLIC void* Balloc_Top(balloc_h bh)
{
    void* result = NULL;

    if (bh) { balloc_t* bp = bh; result = bp->next; }

    return result;
}


PUBLIC void* Balloc_Next(balloc_h bh)
{
    void* result = NULL;

    if (bh) { balloc_t* bp = bh; result = bp->next; }

    return result;
}


PUBLIC size_t Balloc_Left(balloc_h bh)
{
    size_t result = 0;

    if (bh) { balloc_t* bp = bh; result = bp->room_left; }

    return result;
}


PUBLIC size_t Balloc_Size(balloc_h bh)
{
    size_t result = 0;

    if (bh) { balloc_t* bp = bh; result = bp->size; 
    }
    return result;
}


PUBLIC size_t* Balloc_SizeP(balloc_h bh)
{
    size_t* result = 0;

    if (bh) { balloc_t* bp = bh; result = &bp->size; }

    return result;
}


// End of file: balloc.c

