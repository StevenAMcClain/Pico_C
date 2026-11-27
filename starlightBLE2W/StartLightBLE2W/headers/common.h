// File: common.h

#ifndef COMMON_H
#define COMMON_H

// #define BOARD_PICO_W 1
// #define BOARD_PICO2_W 1
// #define BOARD_PICO_W_PLUS 1

#define BOARD_REDBOARD_IOT_RP2350 1

#if defined(BOARD_PICO_W_PLUS) || defined(BOARD_REDBOARD_IOT_RP2350)
#define PICO_USE_PSRAM 1
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define USE_DEBUG /**/

#ifdef USE_DEBUG
#include "debug.h"
#endif //USE_DEBUG

#define PRIVATE static
#define PUBLIC

#define NIL ((void*)0)

#define FOREVER for(;;)

#define BIT(x) (1u << (x))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define AFTER_PTR(x, s) ((void*)(((uint8_t*)(x)) + (s)))

#define MAX_WORD_SIZE 32
#define MAX_DESCRIPTOR_SIZE 32

// BTStack defines.
#define ENABLE_LOG_INFO 0
#define ENABLE_LOG_ERROR 1
#define ENABLE_LOG_DEBUG 0

#define EQUAL 0

#define UNUSED(x) (void)(x)

typedef uint16_t handle_t;

#endif   // COMMON_H

// EndFile: common.h
