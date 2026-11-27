// File: error.h

#ifndef ERROR_H
#define ERROR_H

#define MAX_ERROR_LINES 10
#define MAX_ERROR_SIZE 80
#define ERROR_BUFFER_SIZE (MAX_ERROR_LINES * MAX_ERROR_SIZE)

extern void Error_Clear(void);

extern void Error_Set_Buffer(char* /*buff*/, size_t /*buff_size*/);

extern bool Error_Get_Error_Line(size_t n, char**error_line);

extern bool Error_Posted(void);   // returns true if errors have been posted.

extern void Error_Dump(char* message);

extern void Error_Post(const char* /*fmt*/, ...);

#define POST_ERROR(fmt, ...) Error_Post("%s:"fmt, __func__, ##__VA_ARGS__)

#endif // ERROR_H

// End of file: error.h
