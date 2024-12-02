// File: Common.h

#include "pico/stdlib.h"


#define PRIVATE static
#define PUBLIC

#define FLOAT double

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define AFTER_PTR(x, s) ((void*)(((uint8_t*)(x)) + (s)))

//------------------------
#define DEBUG /**/

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif
//------------------------
#define ERRORS /**/

#ifdef ERRORS
#define E(x) x
#else
#define E(x)
#endif

extern void ObLED_On(void);
extern void ObLED_Off(void);


// EndFile: Common.h
