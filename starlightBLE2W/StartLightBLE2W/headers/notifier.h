// File: notifier.h

#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <stdbool.h>
#include <stdint.h>

extern void Notifier_Initialize(void);      

extern void Notifier_Reset(void);

extern bool Notifier_Post_buffer(uint16_t handle, const char* buffer, uint16_t buffer_size);

extern void Notifier_Post_String(uint16_t handle, const char* str);

extern void Notifier_Process_Can_Send_Now_Event(void);

#endif // NOTIFIER_H

// End of file: notifier.h
