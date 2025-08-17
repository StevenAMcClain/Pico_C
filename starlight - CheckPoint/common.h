// File: Common.h

#include "pico/stdlib.h"


#define PRIVATE static
#define PUBLIC

#define NIL ((void*)0)
#define FLOAT double

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

extern void _Printf(const char *fmt, ...);
#define PRINTF _Printf

#define BTPRINTF BlueTooth_Printf

extern void ObLED_On(void);
extern void ObLED_Off(void);

#define FIRST_CORE_MAIN Start_Parser
#define SECOND_CORE_MAIN Start_BlueTooth

//#define FIRST_CORE_MAIN Start_BlueTooth
//#define SECOND_CORE_MAIN Start_Parser

// EndFile: Common.h
