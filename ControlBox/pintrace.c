// File: pintrace.h

#include "common.h"
#include "pintrace.h"

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"

#ifdef USE_PINTRACE
PUBLIC void PinTrace_Init(void)
{
    GPIOPinTypeGPIOOutput(PINTRACE_PORT, PINTRACE_PIN);
}

PUBLIC void PinTrace_Set(void)
{
    GPIOPinWrite(PINTRACE_PORT, GPIO_PIN_1, PINTRACE_PIN);
}

PUBLIC void PinTrace_Clear(void)
{
    GPIOPinWrite(PINTRACE_PORT, PINTRACE_PIN, 0);
}
#endif

// EndFile: pintrace.c
