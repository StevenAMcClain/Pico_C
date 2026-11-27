// File: error.h

#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#define MAX_ERROR_LINES 10
#define MAX_ERROR_SIZE 80
#define ERROR_BUFFER_SIZE (MAX_ERROR_LINES * MAX_ERROR_SIZE)

extern uint32_t Error_Serial;      // Incremented on every post; compare before/after to detect errors.

extern void Error_Clear(void);

extern void Error_Set_Buffer(char* /*buff*/, size_t /*buff_size*/);

extern bool Error_Posted(void);   // returns true if errors have been posted.

extern void* Error_Post(const char* /*fmt*/, ...);

#define POST_ERROR(fmt, ...) Error_Post("%s:"fmt, __func__, ##__VA_ARGS__)

extern void Error_Dump(const char* message);

extern void Error_Flush(const char* message);

#endif // ERROR_H

// End of file: error.h
