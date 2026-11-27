// File: io_service_bus.c

#include "common.h"
#include "io_service_bus.h"

#include <stdio.h>

#include "error.h"
#include "picopins.h"

PRIVATE uint8_t iunit = 0;


PRIVATE bool IOSERV_Write_Bus(ATT_VALUE* att_value, uint32_t val)
{
    bool result = false;

    if (att_value->iohandle)
    {
        uint8_t* pins = (uint8_t*)att_value->iohandle;
        uint32_t mask = 0x00000001;

        for (int i=0; i < MAX_BUS_PINS; ++i)
        {
            if (*pins == BAD_PIN_NUMBER) { break; }

            gpio_put(*pins, (val & mask));
            result = true;

            mask <<= 1;
            ++pins;
        }
    }
    else { POST_ERROR("%s: no io handle!", __func__); }

    return result;
}


PRIVATE uint16_t IOSERV_Read_Bus(ATT_VALUE* att_value, uint8_t* value, uint16_t value_len)
{
    uint16_t rval = 0;

    if (att_value->iohandle)
    {
        uint8_t* pins = (uint8_t*)att_value->iohandle;
        uint32_t val = 0;

        uint32_t mask = 0x00000001;
        int i;
        for (i=0; i < MAX_BUS_PINS; ++i)
        {
            if (*pins == BAD_PIN_NUMBER) { break; }

            val |= gpio_get(*pins) ? mask : 0;

            mask <<= 1;
            ++pins;
        }

        if (value_len >= 4)
        {
            value[0] =  val;
            value[1] = (val >> 8) & 0xFF;
            value[2] = (val >> 16) & 0xFF;
            value[3] = (val >> 24) & 0xFF;
            
            rval = 4;   // Four byte value.
        }
        else { POST_ERROR("%s: value buffer too small(%p, %d)!", __func__, value, value_len); }
    }
    else { POST_ERROR("%s: no io handle!", __func__); }

    return rval;
}


PRIVATE bool IOSERV_Initialize_OutBus(const char* name, uint8_t* port_pins, size_t pin_count, const char* user_desc)
{
    static int next_bus = 0;
    static uint8_t bus_pins[MAX_BUSSES][MAX_BUS_PINS];

    if (pin_count > MAX_BUS_PINS)
    {
        POST_ERROR("BUS supports max %d pins!", MAX_BUS_PINS);
        return false;
    }

    if (!pin_count)
    {
        POST_ERROR("BUS must have at least one pin!");
        return false;
    }

    bool result = false;

    if (next_bus < MAX_BUSSES)
    {
        int reserved = 0;

        for (size_t i=0; i < pin_count; ++i)
        {
            uint8_t port_pin = port_pins[i];
            
            if (PicoPin_isReserved(&Pico_Pins, port_pin))
            {
                POST_ERROR("Pin %d is already reserved!", port_pin);
                ++reserved;
            }
        }

        if (!reserved)
        {
            handle_t handle = New_Characteristic(name, CAN_WRITE, IOTYPE_OUTBUS, iunit++);

            uint8_t* bus = &bus_pins[next_bus++][0];
            uint8_t* bptr = bus;

            for (size_t i=0; i < pin_count; ++i)
            {
                uint8_t port_pin = port_pins[i];

                *bptr++ = port_pin;

                if (PicoPin_Reserve(&Pico_Pins, port_pin))
                {
                    gpio_init(port_pin);
                    gpio_set_dir(port_pin, GPIO_OUT);    // output on GPIO PORT_PIN
                }
                else { POST_ERROR("Can't reserve pin %d!", port_pin); }
            }
            *bptr = BAD_PIN_NUMBER;

            /*ATT_VALUE* val =*/ Att_Values_Add(handle, name, IOTYPE_OUTBUS, (uintptr_t)bus, 0, 0, user_desc);

            result = true;
        }
    }

    return result;
}


PUBLIC ioserv_function_t IOServ_Output_Bus = 
{
    "OUTBUS",
    IOSERV_Initialize_OutBus,
    IOSERV_Read_Bus,
    IOSERV_Write_Bus
};


// End of file: io_service_bus.c
