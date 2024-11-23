// File: StarLight.c

#include "Common.h"

#include <stdio.h>

#include "bluetooth_stdio.h"

#include "obled.h"
#include "Blob.h"
#include "Led.h"
#include "Parser.h"

#define NUM_PIXELS 19


void main(void)
{
    stdio_init_all();

    printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Server();

    pico_led_init();

    LED_Init(NUM_PIXELS);

    Blob_Init();

    Parser();
}

// EndFile: StarLight.c