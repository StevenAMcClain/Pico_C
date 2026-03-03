// File: controlbox.c

#include <comport.h>
#include "common.h"
#include "controlbox.h"

//#include <stdio.h>
#include <stdint.h>
//#include <stdbool.h>

#ifdef USE_UARTPRINTF
#include "utils/uartstdio.h"
#endif


PUBLIC void CB_Post_Event(CB_TYPE type, int device, int value)
{
    uint32_t event = ((type & CB_TYPE_MASK | device & CB_DEVICE_MASK) << CB_TYPEDEV_SHIFT) | (value & CB_VALUE_MASK);

#ifdef USE_UARTPRINTF
    UARTprintf("type = %x, device = %x, value = %x, event = %x\n", type, device, value, event);
#endif

    SSI_sendData(event);
}


// EndFile: controlbox.c
