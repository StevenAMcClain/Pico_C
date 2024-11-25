// File: StarLight.c

#include "Common.h"

#include <stdio.h>

#include "bluetooth_stdio.h"

#include "obled.h"
#include "Blob.h"
#include "Led.h"
#include "Parser.h"


#define NUM_PIXELS 19       // This is the (initial) actual number LEDs on the string.


void main(void)
{
    stdio_init_all();               // Prepare stdio for use.

    printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Server();       // Start BlueTooth server on second core.
    pico_led_init();                // Prepare pico led for use.
    LED_Init(NUM_PIXELS);           // Prepare LED string driver for use.
    Blob_Init();                    // Get Blob engine ready.
    Start_Parser();                 // Start parsing BlueTooth characters.
}

// EndFile: StarLight.c