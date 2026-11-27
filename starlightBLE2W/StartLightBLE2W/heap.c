/*
 * Modernized Heap Allocator
 * -------------------------
 * A compact best-fit heap with coalescing free list.
 * Preserves the semantics of the 1993 Heap.c allocator,
 * but rewritten for clarity, safety, and modern C style.
 */

#include "common.h"
#include "heap.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "error.h"

#define MIN_HEAP_SIZE  8
#define TAG_FREE       0xF0
#define TAG_USED       0x00

//typedef uint32_t HEAP;

/* Free block header layout:
 *   [0] tag (TAG_FREE)
 *   [1] reserved
 *   [2..3] size (uint16_t)
 *   [4..7] next_free (uint32_t)
 *
 * Total header size = 8 bytes
 */
typedef struct 
{
    uint8_t  tag;
    uint8_t  _pad;
    uint16_t size;
    uint32_t next;

} free_header_t;

typedef struct 
{
    uint8_t *buf;
    HEAP     total;
    HEAP     top;
    HEAP     freelist;

} heap_manager;

/* ---------------------------------------------
 * Utility
 * --------------------------------------------- */

PRIVATE inline free_header_t *fh(uint8_t *base, HEAP idx) {
    return (free_header_t *)(base + idx);
}

PRIVATE inline bool is_free(uint8_t *base, HEAP idx) {
    return fh(base, idx)->tag == TAG_FREE;
}

PRIVATE inline HEAP free_size(uint8_t *base, HEAP idx) {
    return fh(base, idx)->size;
}

PRIVATE inline HEAP free_next(uint8_t *base, HEAP idx) {
    return fh(base, idx)->next;
}

PRIVATE inline void free_set(uint8_t *base, HEAP idx, HEAP size, HEAP next) {
    free_header_t *h = fh(base, idx);
    h->tag  = TAG_FREE;
    h->size = (uint16_t)size;
    h->next = next;
}

/* ---------------------------------------------
 * Initialization
 * --------------------------------------------- */

PUBLIC heap_manager_t *Heap_Init(HEAP total_size)
{
    heap_manager* h = calloc(1, sizeof(*h));
    if (!h) return NULL;

    h->buf = calloc(1, total_size);
    if (!h->buf) {
        free(h);
        return NULL;
    }

    h->total    = total_size - 1;
    h->top      = 1;
    h->freelist = 0;

    return h;
}

PUBLIC void Heap_Terminate(heap_manager_t *h)
{
    if (!h) return;
    heap_manager* hm = (heap_manager*)h;
    free(hm->buf);
    free(hm);
}

/* ---------------------------------------------
 * Length normalization
 * --------------------------------------------- */

PRIVATE HEAP fix_len(HEAP len, const void *src)
{
    if (len == 0 && src)
        len = (HEAP)strlen(src) + 1;

    if (len < MIN_HEAP_SIZE)
        len = MIN_HEAP_SIZE;

    if (len & 1)
        ++len;

    return len;
}

/* ---------------------------------------------
 * Best-fit search
 * --------------------------------------------- */

PRIVATE HEAP search_free(heap_manager_t *h, HEAP len)
{
    heap_manager* hm = (heap_manager*)h;

    HEAP best      = 0;
    HEAP best_size = UINT32_MAX;
    HEAP best_prev = 0;

    HEAP prev = 0;
    HEAP cur  = hm->freelist;

    while (cur)
    {
        HEAP size = free_size(hm->buf, cur);

        if (size >= len && size < best_size)
        {
            best      = cur;
            best_size = size;
            best_prev = prev;
        }

        HEAP next = free_next(hm->buf, cur);

        if (next == cur) { POST_ERROR("Heap freelist loop detected"); return 0; }

        prev = cur;
        cur  = next;
    }

    if (!best) { return 0; }

    HEAP leftover = best_size - len;
    HEAP next     = free_next(hm->buf, best);

    if (leftover >= sizeof(free_header_t) + MIN_HEAP_SIZE)
    {
        HEAP newblk = best + len;

        free_set(hm->buf, newblk, leftover, next);

        if (best_prev) { fh(hm->buf, best_prev)->next = newblk; }
        else           { hm->freelist = newblk; }
    }
    else
    {
        if (best_prev) { fh(hm->buf, best_prev)->next = next; }
        else           { hm->freelist = next; }
    }

    return best;
}
// PRIVATE HEAP search_free(heap_manager_t *h, HEAP len)
// {
//     heap_manager* hm = (heap_manager*)h;
//     HEAP best = 0;
//     HEAP best_size = UINT32_MAX;

//     HEAP prev = 0;
//     HEAP cur  = hm->freelist;

//     while (cur) 
//     {
//         HEAP size = free_size(hm->buf, cur);

//         if (size >= len) 
//         {
//             if (size < best_size) 
//             {
//                 best      = cur;
//                 best_size = size;
//             }
//         }

//         HEAP next = free_next(hm->buf, cur);

//         if (next == cur) {
//             /* circular link protection */
//             return 0;
//         }

//         prev = cur;
//         cur  = next;
//     }

//     if (!best) { return 0; }

//     /* Split block if needed */
//     HEAP leftover = best_size - len;
//     HEAP next     = free_next(hm->buf, best);

//     if (leftover >= MIN_HEAP_SIZE) 
//     {
//         HEAP newblk = best + len;
//         free_set(hm->buf, newblk, leftover, next);

//         /* update freelist link */
//         if (prev)
//             fh(hm->buf, prev)->next = newblk;
//         else
//             hm->freelist = newblk;
//     }
//     else
//     {
//         /* consume entire block */
//         if (prev)
//             fh(hm->buf, prev)->next = next;
//         else
//             hm->freelist = next;
//     }

//     return best;
// }

/* ---------------------------------------------
 * Allocation
 * --------------------------------------------- */

PUBLIC HEAP Heap_Alloc(heap_manager_t *h, HEAP len, const void *src)
{
    heap_manager* hm = (heap_manager*)h;

    len = fix_len(len, src);

    HEAP idx = search_free(h, len);

    if (!idx) 
    {
        /* allocate from top */
        if (hm->top + len > hm->total) { return 0; }

        idx = hm->top;
        hm->top += len;
    }

    if (src) { memcpy(hm->buf + idx, src, len); }
    else     { memset(hm->buf + idx, 0, len);   }

    return idx;
}

/* ---------------------------------------------
 * Delete / Free
 * --------------------------------------------- */

PUBLIC void Heap_Delete(heap_manager_t *h, HEAP idx, HEAP len)
{
    heap_manager* hm = (heap_manager*)h;

    if (idx >= hm->top) { return; }

    len = fix_len(len, hm->buf + idx);
    HEAP end = idx + len;

    /* Case 1: freeing top block → shrink heap */
    if (end == hm->top) 
    {
        hm->top = idx;
        hm->freelist = 0;
        return;
    }

    /* Insert into freelist sorted by address */
    HEAP prev = 0;
    HEAP cur  = hm->freelist;

    while (cur && cur < idx) 
    {
        prev = cur;
        cur  = free_next(hm->buf, cur);
    }

    /* Coalesce with previous */
    if (prev && prev + free_size(hm->buf, prev) == idx) 
    {
        idx = prev;
        len += free_size(hm->buf, prev);
    }

    /* Coalesce with next */
    if (cur && end == cur) 
    {
        len += free_size(hm->buf, cur);
        cur = free_next(hm->buf, cur);
    }

    free_set(hm->buf, idx, len, cur);

    if (prev && prev != idx) { fh(hm->buf, prev)->next = idx; }
    else if (!prev)          { hm->freelist = idx; }
}


/* ---------------------------------------------
 * Utility
 * --------------------------------------------- */

PUBLIC HEAP Heap_Member(heap_manager_t *h, void *ptr)
{
    heap_manager* hm = (heap_manager*)h;
    uintptr_t off = (uint8_t *)ptr - hm->buf;

    if (off >= hm->total) { return 0; }

    return (HEAP)off;
}


PUBLIC HEAP Heap_Used(heap_manager_t *h)
{
    heap_manager* hm = (heap_manager*)h;
    HEAP used = 0;
    HEAP cur  = 0;

    while (cur < hm->top) 
    {
        if (!is_free(hm->buf, cur)) { ++used; }

        ++cur;
    }

    return used;
}


PUBLIC void Heap_Dump(const heap_manager_t *h)
{
    heap_manager* hm = (heap_manager*)h;
    const uint8_t *buf = hm->buf;
    HEAP total = hm->total;
    HEAP top   = hm->top;

    printf("\n=== HEAP ASCII MAP ===\n");
    printf("Total: %lu bytes\n", total);
    printf("Top:   %lu\n", top);
    printf("Free:  %lu\n\n", hm->freelist);

    HEAP cur = 0;

    while (cur < total) 
    {
        /* Header for each 16-byte line */
        printf("%08lX  ", cur);

        /* Hex dump */
        for (int i = 0; i < 16; i++) 
        {
            HEAP idx = cur + i;

            if (idx < total) { printf("%02X ", buf[idx]); }
            else             { printf("   "); }
        }

        printf(" | ");

        /* ASCII dump */
        for (int i = 0; i < 16; i++) 
        {
            HEAP idx = cur + i;

            if (idx < total) 
            {
                uint8_t c = buf[idx];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            else { printf(" "); }
        }
        printf("\n");

        cur += 16;
    }

    printf("\n=== HEAP STRUCTURE ===\n");

    /* Walk freelist */
    // HEAP f = h->freelist;
    // while (f) {
    //     const free_header_t *fh = (const free_header_t *)(buf + f);
    //     printf("Free block @ %u: size=%u next=%u\n",
    //            f, fh->size, fh->next);
    //     f = fh->next;
    // }

    printf("=======================\n\n");
}

// EndFile: Heap.c
