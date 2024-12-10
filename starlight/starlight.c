// File: StarLight.c

#include "Common.h"

#include <stdio.h>

#include "hardware/pio.h"

#include "bluetooth_stdio.h"

#include "obled.h"
#include "Blob.h"
#include "Led.h"
#include "Parser.h"

#include "ws2812.h"

//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};

void main(void)
{
    stdio_init_all();               // Prepare stdio for use.

    printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Server();

    LED_Init();           // Prepare LED strings driver for use.

    Blob_Init();

    // PHY_Set_led_count(1, 19);
    // LEDS_Set_Phynum(1);
    // LEDS_All_Black();

    Start_Parser();  // Never returns.
}


// EndFile: StarLight.c