// File: watcher.h

#ifndef WATCHER_H
#define WATCHER_H

#include "common.h"

#define MAX_WATCHER_RECORDS 10
#define MAX_WATCHER_BUFFER_SIZE 80
#define WATCHER_BUFFER_SIZE (MAX_WATCHER_RECORDS * MAX_WATCHER_BUFFER_SIZE)


extern void Watcher_Clear(void);

extern void Watcher_Set_Buffer(char* /*buff*/, size_t /*buff_size*/);

extern bool Watcher_Get_Watcher_Line(size_t n, char** watcher_line);

extern bool Watcher_Posted(void);   // returns true if watchers have been posted.

extern void Watcher_Dump(void);

extern void Watcher_Post(const char* /*fmt*/, ...);

extern void Watcher_Flush(void);

#endif // WATCHER_H

// End of file: watcher.h
