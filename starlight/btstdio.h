// File: bluetooth_stdio.h

#ifndef BLUETOOTH_STDIO_H
#define BLUETOOTH_STDIO_H

#define QUEUE_SIZE 100

extern void Start_BlueTooth_Core(void);

extern char BlueTooth_GetChar();
extern bool BlueTooth_Check_Receive(void);
extern int BlueTooth_TryGetChar();    // Try to getchar. Returns char or PICO_ERROR_TIMEOUT.

extern void BlueTooth_Send_Buffer(uint8_t* buff, size_t n);
#define BlueTooth_Send_String(str)    BlueTooth_Send_Buffer((uint8_t* )str, strlen(str));

extern void BlueTooth_Printf(const char *fmt, ...);

#endif // BLUETOOTH_STDIO_H

// EndFile: bluetooth.h
