// File: heap.h

#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t HEAP;
typedef void heap_manager_t;

extern heap_manager_t *Heap_Init(HEAP total_size);
extern void Heap_Terminate(heap_manager_t *h);
extern HEAP Heap_Alloc(heap_manager_t *h, HEAP len, const void *src);
extern void Heap_Delete(heap_manager_t *h, HEAP idx, HEAP len);
extern HEAP Heap_Member(heap_manager_t *h, void *ptr);
extern HEAP Heap_Used(heap_manager_t *h);
extern void Heap_Dump(const heap_manager_t *h);

#endif // HEAP_H

// End of file: heap.h
