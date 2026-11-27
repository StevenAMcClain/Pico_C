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

PRIVATE size_t Error_Count = 0;


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


PRIVATE bool Error_Get_Error_Line(size_t n, char**error_line)
{
    bool result = false;

    if (n < Error_Count)
    {
        *error_line = Error_Table[n];
        result = true;
    }

    return result;
}


PUBLIC uint32_t Error_Serial = 0;


PUBLIC void* Error_Post(const char* fmt, ...)
{
    char msg[MAX_ERROR_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    ++Error_Serial;                         // Count it even if it can't be stored.

    size_t len = strlen(msg) + 1;           // Including the null.

    if (len == 1) { return NULL; }               // Empty message: ignore.

    printf("%s\n", msg);

    if (!Error_Buff || Error_Buff_Size < 2) { return NULL; }     // No buffer configured: console only.

    if (len > Error_Buff_Size)              // Message can never fit: truncate it.
    {
        len = Error_Buff_Size;
        msg[len - 1] = 0;
    }

    if (len > Error_Buff_Free || Error_Count >= MAX_ERROR_LINES)
    {
        printf("!!!! Error buffer full: flushing !!!!\n");

        Error_Dump(":::Emptying error buffers:::\n");
        Error_Clear();                      // Now Error_Buff_Free == Error_Buff_Size >= len.
    }

    memcpy(Error_BPtr, msg, len);
    Error_Table[Error_Count++] = Error_BPtr;

    Error_BPtr += len;
    Error_Buff_Free -= len;

    return NULL;
}


PUBLIC void Error_Dump(const char* message)
{
    bool is_first = true;
    int n = 0;
    char* error_line;

    while (Error_Get_Error_Line(n, &error_line))
    {
        if (is_first)
        {
            is_first = false;
            Post_Output(message, 0);
            printf("%s\n", message);
        }

        Post_Output(error_line, 0);
        printf("*** Error[%d]: %s\n", n, error_line);
        Post_Output("\n", 1);
        ++n;
    }
}


PUBLIC void Error_Flush(const char* message)
{
    if (Error_Count > 0) { Error_Dump(message); }
    Error_Clear();
}


// End of file: error.c
