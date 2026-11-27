// File: debug_pin.h.c

#include "common.h"
#include "debug_pin.h"

#include <stdio.h>

#include <pico/stdlib.h>

#include "error.h"
#include "picopins.h"


PRIVATE uint8_t debug_port_pin = BAD_PIN_NUMBER;


PUBLIC void Debug_Pin_Initialize(uint8_t pin)
{
    if (PicoPin_Reserve(&Pico_Pins, pin))
    {
        debug_port_pin = pin;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);    // input on GPIO PORT_PIN
    }
    else { POST_ERROR("Can't reserve pin %d.\n", pin); }
}


PUBLIC void Debug_Pin_Set(bool state)
{
    if (debug_port_pin != BAD_PIN_NUMBER)
    {
        gpio_put(debug_port_pin, state);
    }
}


// End of file: debug_pin.c
