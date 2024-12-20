// File: StarLight.c

#include "common.h"

#include <stdio.h>

#include "bluetooth_stdio.h"

#include "blob.h"
#include "debug.h"
#include "led.h"
#include "obled.h"
#include "parser.h"


PUBLIC uint32_t Debug_Mask = DEBUG_ALL;

//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};

void main(void)
{
    stdio_init_all();           // Prepare stdio for use.

    printf("\r\n\nStartlight %s %s %s: startup.\n", 
                         __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Core();     // Startup server on second core.
    LED_Init();                 // Prepare LED strings PIO driver for use.
    Blob_Init();                // Get blob engine ready to run.
    Start_Parser();             // Parse bluetooth stream..... Never return.
}


// EndFile: StarLight.c