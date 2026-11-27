// File: io_service_led.c

#include "common.h"
#include "io_service_led.h"

#include <stdio.h>

#include "error.h"
#include "picopins.h"
#include "picopwm.h"

PRIVATE uint8_t iunit = 0;

PRIVATE uint16_t scale_255_to_1000(uint8_t x)
{
    return (uint16_t)((x * 1000 + 127) >> 8);   // divide by 256 instead of 255
}


PRIVATE uint8_t scale_1000_to_255(uint16_t x)
{
    // clamp input to valid range
    if (x > 1000) x = 1000;

    // inverse of (x * 1000) >> 8
    // y = (x * 256) / 1000, with rounding
    return (uint8_t)((x * 256 + 500) / 1000);
}


PRIVATE bool IOSERV_Write_Led(ATT_VALUE* att_value, uint32_t val)
{
    bool result = false;

    if (att_value->iohandle)
    {
        uint32_t pins = (uint32_t)att_value->iohandle;

        uint8_t   red_pin =  pins        & 0xFF;
        uint8_t green_pin = (pins >> 8)  & 0xFF;
        uint8_t  blue_pin = (pins >> 16) & 0xFF;
        uint8_t white_pin = (pins >> 24) & 0xFF;

        uint8_t   red_val =  val        & 0xFF;
        uint8_t green_val = (val >> 8)  & 0xFF;
        uint8_t  blue_val = (val >> 16) & 0xFF;
        uint8_t white_val = (val >> 24) & 0xFF;

        if (  red_pin != BAD_PIN_NUMBER)   { PicoPWM_Set_Bright(  red_pin, scale_255_to_1000(  red_val)); result = true; }
        if (green_pin != BAD_PIN_NUMBER)   { PicoPWM_Set_Bright(green_pin, scale_255_to_1000(green_val)); result = true; }
        if ( blue_pin != BAD_PIN_NUMBER)   { PicoPWM_Set_Bright( blue_pin, scale_255_to_1000( blue_val)); result = true; }
        if (white_pin != BAD_PIN_NUMBER)   { PicoPWM_Set_Bright(white_pin, scale_255_to_1000(white_val)); result = true; }
    }
    else { POST_ERROR("No IO handle!"); }

    return result;
}


PRIVATE uint16_t IOSERV_Read_Led(ATT_VALUE* att_value, uint8_t* value, uint16_t value_len)
{
    uint16_t rval = 0;

    if (att_value->iohandle)
    {
        if (value && value_len >= 4)
        {
            uint32_t pins = (uint32_t)att_value->iohandle;

            uint8_t   red_pin =  pins        & 0xFF;
            uint8_t green_pin = (pins >> 8)  & 0xFF;
            uint8_t  blue_pin = (pins >> 16) & 0xFF;
            uint8_t white_pin = (pins >> 24) & 0xFF;

            uint8_t   red_val = (  red_pin != BAD_PIN_NUMBER) ? scale_1000_to_255(PicoPWM_Get_Bright(  red_pin)) : 0;
            uint8_t green_val = (green_pin != BAD_PIN_NUMBER) ? scale_1000_to_255(PicoPWM_Get_Bright(green_pin)) : 0;
            uint8_t  blue_val = ( blue_pin != BAD_PIN_NUMBER) ? scale_1000_to_255(PicoPWM_Get_Bright( blue_pin)) : 0;
            uint8_t white_val = (white_pin != BAD_PIN_NUMBER) ? scale_1000_to_255(PicoPWM_Get_Bright(white_pin)) : 0;

            DBG(0, DEBUG_BLESERVICE, "pins %d %d %d %d, vals %d %d %d %d\n", red_pin, green_pin, blue_pin, white_pin, red_val, green_val, blue_val, white_val);

            value[0] = red_val;
            value[1] = green_val;
            value[2] = blue_val;
            value[3] = white_val;
        }
        else { if (value) { POST_ERROR("Value buffer too small(%p, %d)!", value, value_len); } }

        rval = 4;   // Four bytes value.
    }
    else { POST_ERROR("No io handle!"); }

    return rval;
}


PRIVATE bool IOSERV_Initialize_Led(const char* name, uint8_t* port_pins, size_t pin_count, const char* user_desc)
{
    UNUSED(pin_count);

    bool result = false;

    int reserved = 0;

    int i;
    for (i=0; i < NUM_LED_PINS; ++i)
    {
        uint8_t port_pin = port_pins[i];
        
        if (port_pin != BAD_PIN_NUMBER)
        {
            if (PicoPin_isReserved(&Pico_Pins, port_pin))
            {
                POST_ERROR("Pin %d is already reserved!", port_pin);
                ++reserved;
            }
        }
    }

    if (!reserved)
    {
        handle_t handle = New_Characteristic(name, CAN_WRITE, 0, 0, 0, IOTYPE_LED, iunit++);

        // Encoded as 4 bytes with pin numbers for [red, green, blue, white].
        //
        uint32_t led_pins = port_pins[0] | (port_pins[1] << 8) | (port_pins[2] << 16) | (port_pins[3] << 24);

        DBG(0, DEBUG_BLESERVICE, "Pins ");

        for (i=0; i < NUM_LED_PINS; ++i)
        {
            uint8_t port_pin = port_pins[i];

            if (port_pin != BAD_PIN_NUMBER)
            {
                PicoPin_Reserve(&Pico_Pins, port_pin);
                PicoPWM_Initialize(port_pin, USE_GAMMA);
                // printf("%d ", port_pin);
            }
        }
        // printf("\n");

        /*ATT_VALUE* val =*/ Att_Values_Add(handle, name, IOTYPE_LED, (uintptr_t)led_pins, 0, 0, user_desc);

        result = true;
    }

    return result;
}


PUBLIC ioserv_function_t IOServ_Led = 
{
    "LED",
    IOSERV_Initialize_Led,
    IOSERV_Read_Led,
    IOSERV_Write_Led
};


// End of file: io_service_led.c
