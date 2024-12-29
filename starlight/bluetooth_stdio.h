// File: bluetooth_stdio.h

#ifndef BLUETOOTH_STDIO_H
#define BLUETOOTH_STDIO_H

#define QUEUE_SIZE 100

extern void Start_BlueTooth_Core(void);

extern char BlueTooth_GetChar();
extern bool BlueTooth_Check_Receive(void);

extern void BlueTooth_Send_String(char* str);
extern void BlueTooth_Printf(const char *fmt, ...);
extern void BlueTooth_Printf_faster(const char *fmt, ...);

#endif // BLUETOOTH_STDIO_H

// EndFile: bluetooth.h
