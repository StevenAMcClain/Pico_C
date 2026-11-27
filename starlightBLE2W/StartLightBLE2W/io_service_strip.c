// File: io_service_strip.c


#include "common.h"
#include "io_service_strip.h"

#include <stdio.h>

#include "error.h"
#include "picopins.h"
#include "ws2812.h"

PRIVATE uint8_t iunit = 0;

PRIVATE bool IOSERV_Write_Strip(ATT_VALUE* att_value, uint32_t val)
{
    UNUSED(val);   // For now, we don't support writing to the strip characteristic.
    bool result = false;

    if (att_value->iohandle)
    {
//        int pin = (uint)att_value->iohandle;

//        PicoPWM_Set_Bright(pin, val);
        result = true;
    }
    else { POST_ERROR("No io handle!"); }

    return result;
}


PRIVATE uint16_t IOSERV_Read_Strip(ATT_VALUE* att_value, uint8_t* value, uint16_t value_len)
{
    uint16_t rval = 0;

    if (att_value->iohandle)
    {
        int pin = (uint)att_value->iohandle;
//        int val = PicoPWM_Get_Bright(pin);
        int val = 666;

        DBG(0, DEBUG_BLESERVICE, "<PWM> pin %d, value %d, value_len %d\n", pin, val, value_len);

        value[0] = val;
        value[1] = (val >> 8) & 0xFF;
        rval = 2;   // Two byte value.
    }
    else { POST_ERROR("No io handle!"); }

    return rval;
}


PUBLIC bool IOSERV_Initialize_Strip(char*name, uint8_t port_pin, uint32_t num_leds, bool is_rgbw, char* user_desc)
{
    if (PicoPin_Reserve(&Pico_Pins, port_pin))
    {
        handle_t handle = New_Characteristic(name, CAN_WRITE, 0, 0, 0, IOTYPE_STRIP, iunit++);

        WS2812_Setup_Pio(port_pin, num_leds, is_rgbw);
 //        PicoPWM_Initialize(port_pin, USE_LINEAR);

        /* ATT_VALUE* val = */ Att_Values_Add(handle, name, IOTYPE_STRIP, (uintptr_t)port_pin, 0, 0, user_desc);
        return true;
    }
    else { POST_ERROR("Can't reserve port pin %d!", port_pin); }

    return false;
}


PUBLIC ioserv_function_t IOServ_Strip = 
{
    "STRIP",
    (void*)IOSERV_Initialize_Strip,
    IOSERV_Read_Strip,
    IOSERV_Write_Strip
};


// End of file: io_service_strip.c
