// File: bluetooth_stdio.h

#ifndef BLUETOOTH_STDIO_H
#define BLUETOOTH_STDIO_H

extern void Start_BlueTooth(void);

extern void Bt_Make_Request(uint8_t* buff, size_t size);
extern size_t Bt_ChecK_Request(void);
extern void Bt_Cancel_Request(void);

extern void BlueTooth_Send_String(char* str);
extern void BlueTooth_Printf(const char *fmt, ...);

#endif // BLUETOOTH_STDIO_H

// EndFile: bluetooth.h
