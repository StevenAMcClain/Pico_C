/*
 * Common.h -  Useful stuff that is used every where.
 *
 *  Created on: Oct. 9, 2025
 *      Author: Steven
 */

#ifndef COMMON_H_
#define COMMON_H_

#define VERSION "V1.0a"

#define PUBLIC
#define PRIVATE static

#define BIT(x) (1 << (x))
#define ABS(x) (((x) >= 0) ? (x) : -(x))

#define NIL ((void*)0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

//#define USE_PRINTF
//#define USE_UARTPRINTF

#endif /* COMMON_H_ */
