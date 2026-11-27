// File: picopins.c

#include "common.h"

#include "picopins.h"


//#define PICOPINS_RESERVED 0xFF800000
#define PICOPINS_RESERVED 0x0000000000000F33ULL


PRIVATE void start_critical(picopins_t* pins)
{
    critical_section_enter_blocking(&pins->picopins_cs);
}

PUBLIC gpio_mask_t PicoPin_Set(picopins_t* pins, gpio_mask_t new)
{
    start_critical(pins);
//    critical_section_enter_blocking(&pins->picopins_cs);

    gpio_mask_t old = pins->picopins;

    pins->picopins = new;

    critical_section_exit(&pins->picopins_cs);

    return old;
}


PUBLIC gpio_mask_t PicoPin_Reset(picopins_t* pins)
{
    return PicoPin_Set(pins, PICOPINS_RESERVED);
}


PUBLIC void PicoPin_Initialize(picopins_t* pins)
{
    critical_section_init(&pins->picopins_cs);
    PicoPin_Reset(pins);
}


PRIVATE gpio_mask_t picopin_mask(uint8_t pin)
{
    return (pin <= PICOPIN_MAX) ? 1 << pin : 0;
}


PUBLIC bool PicoPin_Reserve(picopins_t* pins, uint8_t pin)
{
    bool result = false;
    gpio_mask_t mask = picopin_mask(pin);

    if (mask)
    {
        start_critical(pins);
//      critical_section_enter_blocking(&pins->picopins_cs);
        if (mask && !(pins->picopins & mask))
        {
            pins->picopins |= mask;
            result = true;
        }
        critical_section_exit(&pins->picopins_cs);
    }

    return result;
}


PUBLIC void PicoPin_Release(picopins_t* pins, uint8_t pin)
{
    gpio_mask_t mask = picopin_mask(pin);

    if (mask) 
    {
        start_critical(pins);
//        critical_section_enter_blocking(&pins->picopins_cs);
        pins->picopins &= ~mask; 
        critical_section_exit(&pins->picopins_cs);
    }
}


PUBLIC bool PicoPin_isReserved(picopins_t* pins, uint8_t pin)
{
    bool result = false;
    gpio_mask_t mask = picopin_mask(pin);

    if (mask) { result = pins->picopins & mask; }

    return result;
}


// EndFile: picopins.c
