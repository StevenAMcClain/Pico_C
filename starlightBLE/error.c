// File: error.c

#include "common.h"
#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

// Todo: No reentrancy/thread-safety guards on the shared globals (Error_Table, Error_BPtr, etc.)

PRIVATE size_t Error_Buff_Size = 0;

PRIVATE char* Error_Buff = NULL;

PRIVATE size_t Error_Buff_Free = 0;
PRIVATE char* Error_BPtr = NULL;

PRIVATE char* Error_Table[MAX_ERROR_LINES] = {0};

PUBLIC size_t Error_Count = 0;


PUBLIC void Error_Clear(void)
{
    if (Error_Buff) { *Error_Buff = 0; }

    Error_BPtr = Error_Buff;
    Error_Buff_Free = Error_Buff_Size;

    Error_Count = 0;

    for (int i=0; i < MAX_ERROR_LINES; ++i)
    {
        Error_Table[i] = NULL;
    }
}


PUBLIC void Error_Set_Buffer(char* buff, size_t buff_size)
{
    Error_Buff_Size = buff_size;
    Error_Buff = buff;
    Error_Clear();
}


PUBLIC bool Error_Posted(void)
{
    return Error_Count > 0;
}


PUBLIC bool Error_Get_Error_Line(size_t n, char**error_line)
{
    bool result = false;

    if (n < Error_Count)
    {
        *error_line = Error_Table[n];
        result = true;
    }

    return result;
}


PUBLIC void Error_Post(const char* fmt, ...)
{
    if (Error_Buff_Free && Error_Count < MAX_ERROR_LINES)
    {
        Error_Table[Error_Count++] = Error_BPtr;

        char *ptr = Error_BPtr;

        va_list args;
        va_list args_copy;

        va_start(args, fmt);
        va_copy(args_copy, args);

        // First use: format into buffer
        vsnprintf(ptr, Error_Buff_Free, fmt, args);

        // Second use: print to console
        vprintf(fmt, args_copy);
        printf("\n");

        va_end(args);
        va_end(args_copy);

        size_t len = strlen(ptr) + 1;   // Advance past null.

        if (len == 1)  // Empty message?
        {
            --Error_Count;    // Cancel table entry.
        }
        else
        {
            Error_BPtr += len;
            Error_Buff_Free -= len;
        }
    }
    else
    {
        if (!Error_Buff_Free)               { printf("!!!! Error Buffer Full !!!!\n"); }
        if (Error_Count >= MAX_ERROR_LINES) { printf("!!!! Error Table Full !!!!\n"); }
    }
}


PUBLIC void Error_Dump(char* message)
{
    bool is_first = true;
    int n = 0;
    char* error_line;

    while (Error_Get_Error_Line(n, &error_line))
    {
        if (is_first)
        {
            is_first = false;
            Post_Output("*** ", 1);
            Post_Output(message, strlen(message));
            printf("%s\n", message);
        }

        Post_Output(error_line, strlen(error_line));
        printf("*** Error[%d]: %s\n", n, error_line);
        Post_Output("\n", 1);
        ++n;
    }
}


// End of file: error.c
