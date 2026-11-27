// File: debug.h

#ifndef DEBUG_H
#define DEBUG_H

//#include "common.h"
#include <stdio.h>

extern uint32_t Debug_Mask;
extern uint32_t Debug_Level;

#define DEBUG_PRINTF     BIT(0)
#define DEBUG_BUSY       BIT(1)
#define DEBUG_BLOB       BIT(2)
#define DEBUG_BALLOC     BIT(3)
#define DEBUG_PARSER     BIT(4)
#define DEBUG_CONFIG     BIT(5)
#define DEBUG_SCENES     BIT(6)
#define DEBUG_BLUETOOTH  BIT(7)
#define DEBUG_BLESERVICE BIT(8)
#define LEXER_DEBUG_DUMP BIT(9)
#define DEBUG_COMMAND    BIT(10)
#define DEBUG_RESULTS    BIT(11)

#define DEBUG_ALL ((uint32_t)(-1))

#ifdef USE_DEBUG
#define D(m, x) do {if (((m) & Debug_Mask) == (m)) {x;}} while (0)
#define DBG(p, m, fmt, ...) do {if ((((m) & Debug_Mask) == (m)) && (Debug_Level) > (p)) {printf("%s:"fmt, __func__, ##__VA_ARGS__);}} while (0)
#else
#define D(m, x)
#define DBG(p, m, fmt, ...)
#endif

#define DEBUG_MASK_INITIAL DEBUG_ALL        // Initial value for Debug_Mask.
#define DEBUG_MASK_STARTUP 0                // Value for Debug_Mask during startup.
#define DEBUG_MASK_RUNNING DEBUG_ALL        // Value for Debug_Mask when (initially) running.

#define DEBUG_LEVEL_MAX (-1)
#define DEBUG_LEVEL_INITIAL DEBUG_LEVEL_MAX  // Initial value for Debug_Level.
#define DEBUG_LEVEL_STARTUP 2                // Value for Debug_Level during startup.
#define DEBUG_LEVEL_RUNNING 2                // Value for Debug_Level when (initially) running.

#endif // DEBUG_H

// EndFile: debug.h
