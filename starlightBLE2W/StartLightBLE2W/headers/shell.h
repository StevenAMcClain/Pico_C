// File: shell.h

#ifndef SHELL_H
#define SHELL_H

#include "common.h"

extern void Post_Output(const char* buffer, uint16_t buffer_size);
extern void Post_Event(const char* buffer, uint16_t buffer_size);

extern void Shell(void);

#endif // SHELL_H

// End of file: shell.h
