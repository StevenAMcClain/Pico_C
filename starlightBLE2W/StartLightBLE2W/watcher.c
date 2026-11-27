// File: watcher.c

#include "common.h"
#include "watcher.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"

// Todo: No reentrancy/thread-safety guards on the shared globals (Watcher_Table, Watcher_BPtr, etc.)

PRIVATE size_t Watcher_Buff_Size = 0;

PRIVATE char* Watcher_Buff = NULL;

PRIVATE size_t Watcher_Buff_Free = 0;
PRIVATE char* Watcher_BPtr = NULL;

PRIVATE char* Watcher_Table[MAX_WATCHER_RECORDS] = {0};

PRIVATE size_t watcher_count = 0;

PRIVATE char watcher_msg[MAX_WATCHER_BUFFER_SIZE];       // Static: keeps big buffers off the stack.

PRIVATE volatile bool watcher_dump_lock = false;         // Set while Watcher_Dump is iterating.
PRIVATE volatile bool watcher_post_lock = false;         // Set while Watcher_Post is running.

PRIVATE uint32_t watcher_dropped   = 0;                  // Messages dropped due to re-entrancy.
PRIVATE uint32_t watcher_truncated = 0;                  // Messages that were cut short.


// Replace the tail of a truncated, null-terminated string of 'len' bytes
// (including the null) with "...".
PRIVATE void watcher_mark_truncated(char* s, size_t len)
{
    if (len >= 4)
    {
        s[len - 4] = '.';
        s[len - 3] = '.';
        s[len - 2] = '.';
    }
    s[len - 1] = 0;

    ++watcher_truncated;
}


PUBLIC void Watcher_Clear(void)
{
    if (Watcher_Buff && Watcher_Buff_Size > 0) { *Watcher_Buff = 0; }

    Watcher_BPtr = Watcher_Buff;
    Watcher_Buff_Free = Watcher_Buff_Size;

    watcher_count = 0;

    memset(Watcher_Table, 0, sizeof(Watcher_Table));
}


PUBLIC void Watcher_Set_Buffer(char* buff, size_t buff_size)
{
    Watcher_Buff_Size = buff_size;
    Watcher_Buff = buff;
    Watcher_Clear();
}


PUBLIC bool Watcher_Posted(void)
{
    return watcher_count > 0;
}


PUBLIC bool Watcher_Get_Watcher_Line(size_t n, char** watcher_line)
{
    if (!watcher_line) return false;

    *watcher_line = NULL;

    if (n >= watcher_count) { return false; }

    *watcher_line = Watcher_Table[n];

    return true;
}


PUBLIC void Watcher_Post(const char* fmt, ...)
{
    // Re-entrant call (from Post_Event during a dump, or from within Post itself):
    // posting now could clear/overwrite the lines Watcher_Dump is iterating. Drop it.
    if (watcher_dump_lock || watcher_post_lock)
    {
        ++watcher_dropped;
        return;
    }

    watcher_post_lock = true;

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(watcher_msg, sizeof(watcher_msg), fmt, args);
    va_end(args);

    size_t len = strlen(watcher_msg) + 1;       // Including the null.

    if (len == 1) { goto done; }                // Empty message: ignore.

    if (n < 0 || (size_t)n >= sizeof(watcher_msg))      // vsnprintf cut it short.
    {
        watcher_mark_truncated(watcher_msg, len);
    }

    if (!Watcher_Buff || Watcher_Buff_Size < 2) { goto done; }   // No buffer configured.

    if (len > Watcher_Buff_Size)                // Message can never fit: truncate it.
    {
        len = Watcher_Buff_Size;
        watcher_mark_truncated(watcher_msg, len);
    }

    if (len > Watcher_Buff_Free || watcher_count >= MAX_WATCHER_RECORDS)
    {
        printf("!!!! Watcher buffer full: flushing !!!!\n");
        Watcher_Dump();                         // Nested Posts are dropped while this runs.
        Watcher_Clear();                        // Now Watcher_Buff_Free == Watcher_Buff_Size >= len.
    }

    memcpy(Watcher_BPtr, watcher_msg, len);
    Watcher_Table[watcher_count++] = Watcher_BPtr;

    Watcher_BPtr += len;
    Watcher_Buff_Free -= len;

done:
    watcher_post_lock = false;
}


PUBLIC void Watcher_Dump(void)
{
    watcher_dump_lock = true;

    size_t n = 0;
    char* watcher_line;

    // Lines are emitted exactly as stored, with no separator added. If Post_Event
    // doesn't terminate/separate messages, the lines will run together.
    while (Watcher_Get_Watcher_Line(n, &watcher_line))
    {
        Post_Event(watcher_line, strlen(watcher_line));
        ++n;
    }

    if (watcher_dropped || watcher_truncated)
    {
        char note[64];
        int len = snprintf(note, sizeof(note), "[watcher: %lu dropped, %lu truncated]",
                           (unsigned long)watcher_dropped, (unsigned long)watcher_truncated);

        if (len > 0) { Post_Event(note, (size_t)len < sizeof(note) ? (size_t)len : sizeof(note) - 1); }

        watcher_dropped   = 0;
        watcher_truncated = 0;
    }

    //Post_Event("\n", 1);

    watcher_dump_lock = false;
}


PUBLIC void Watcher_Flush(void)
{
    if (watcher_count > 0) { Watcher_Dump(); }
    Watcher_Clear();
}

// End of file: watcher.c
