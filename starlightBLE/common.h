// File: Common.h

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "pico/stdlib.h"

#define PRIVATE static
#define PUBLIC

#define NIL ((void*)0)
#define FLOAT float

#define FOREVER for(;;)

#define BIT(x) (1<<(x))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define AFTER_PTR(x, s) ((void*)(((uint8_t*)(x)) + (s)))

//------------------------
#define ERRORS /**/

#ifdef ERRORS
#define E(x) x
#else
#define E(x)
#endif


// extern void _Printf(const char *fmt, ...);
// #define PRINTF _Printf

// extern void ObLED_On(void);
// extern void ObLED_Off(void);

#define MAX_WORD_SIZE 20
#define MAX_DESCRIPTOR_SIZE 32

// BTStack defines.
#define ENABLE_LOG_INFO 0
#define ENABLE_LOG_ERROR 1
#define ENABLE_LOG_DEBUG 0

#define EQUAL 0

#define UNUSED(x) (void)(x)

typedef uint16_t handle_t;

#endif   // COMMON_H

// EndFile: Common.h
