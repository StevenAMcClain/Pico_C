// File: io_service_pwm.c

#include "common.h"
#include "io_service_pwm.h"

#include <stdio.h>

#include "error.h"
#include "picopins.h"
#include "picopwm.h"

PRIVATE uint8_t iunit = 0;

PRIVATE bool IOSERV_Write_PWM(ATT_VALUE* att_value, uint32_t val)
{
    bool result = false;

    if (att_value->iohandle)
    {
        int pin = (uint)att_value->iohandle;

        PicoPWM_Set_Bright(pin, val);
        result = true;
    }
    else { POST_ERROR("no io handle!"); }

    return result;
}


PRIVATE uint16_t IOSERV_Read_PWM(ATT_VALUE* att_value, uint8_t* value, uint16_t value_len)
{
    uint16_t rval = 0;

    if (att_value->iohandle)
    {
        int pin = (uint)att_value->iohandle;
        int val = PicoPWM_Get_Bright(pin);

        DBG(0, DEBUG_BLESERVICE, "<PWM> pin %d, value %d, value_len %d\n", pin, val, value_len);

        if (value_len >= 2)
        {
            value[0] = val & 0xFF;
            value[1] = (val >> 8) & 0xFF;
            rval = 2;   // Two byte value.
        }
        else { POST_ERROR("value buffer too small(%p, %d)!", value, value_len); }
    }
    else { POST_ERROR("no io handle!"); }

    return rval;
}


PUBLIC bool IOSERV_Initialize_PWM(const char* name, uint8_t* port_pins, size_t pin_count, const char* user_desc)
{
    UNUSED(pin_count);
    
    uint8_t port_pin = *port_pins;

    if (PicoPin_Reserve(&Pico_Pins, port_pin))
    {
        handle_t handle = New_Characteristic(name, CAN_WRITE, GATT_UNIT_PERCENTAGE, GATT_FMT_UINT16, GATT_EXP_DECI, IOTYPE_PWM, iunit++);

        PicoPWM_Initialize(port_pin, USE_LINEAR);

        /* ATT_VALUE* val = */ Att_Values_Add(handle, name, IOTYPE_PWM, (uintptr_t)port_pin, 0, 0, user_desc);
        return true;
    }
    else { POST_ERROR("Can't reserve port pin %d!", port_pin); }

    return false;
}


PUBLIC ioserv_function_t IOServ_PWM = 
{
    "PWM",
    IOSERV_Initialize_PWM,
    IOSERV_Read_PWM,
    IOSERV_Write_PWM
};


// End of file: io_service_pwm.c
