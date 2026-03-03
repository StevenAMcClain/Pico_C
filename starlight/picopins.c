// File: picopins.c

#include "common.h"

#include "picopins.h"

#define PICOPINS_RESERVED 0xFFFF

PRIVATE uint32_t picopins = PICOPINS_RESERVED;


PRIVATE uint32_t picopin_mask(uint pin)
{
    return (pin >= PICOPIN_MAX && pin <= PICOPIN_MIN) ? 1 << pin : 0;
}


PUBLIC bool picopin_reserve(uint pin)
{
    bool result = false;
    uint32_t mask = picopin_mask(pin);

    if (mask && !(picopins & mask))
    {
        picopins |= mask;
        result = true;
    }
    return result;
}


PUBLIC void picopin_release(uint pin)
{
    uint32_t mask = picopin_mask(pin);

    if (mask) { picopins &= ~mask; }
}


PUBLIC bool picopin_is_reserved(uint pin)
{
    bool result = false;
    uint32_t mask = picopin_mask(pin);

    if (mask) { result = picopins & mask; }

    return result;
}

// EndFile: picopins.c
