// File: BLECore.h

#ifndef BLECORE_H
#define BLECORE_H

#include "common.h"

extern void BLECore_main(void);
extern void BLE_Set_Node_Name(char* name);

#define MAX_SCAN_RESP_DATA_SIZE 31
#define MAX_NODE_NAME_SIZE (MAX_SCAN_RESP_DATA_SIZE - 8)   // Leave room for (Len, Type) * 2, 2 byte Company ID, and 2 byte Payload.

extern volatile handle_t connection_handle;

#define MAX_MTU_SIZE 247

#endif // BLECORE_H

// End of file: BLECore.h
