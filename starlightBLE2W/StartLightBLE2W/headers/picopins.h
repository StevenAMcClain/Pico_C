// File: picopins.h

#ifndef PICOPINS_H
#define PICOPINS_H

#include <pico/critical_section.h>

#define PICOPIN_MIN 0
#define PICOPIN_MAX 31

#define BAD_PIN_NUMBER 0xFF

typedef uint64_t gpio_mask_t;

typedef struct picopins
{
    critical_section_t picopins_cs;

    gpio_mask_t picopins;

} picopins_t;

extern picopins_t Pico_Pins;    // Master pin set.

extern void PicoPin_Initialize(picopins_t* pins);

extern gpio_mask_t PicoPin_Reset(picopins_t* pins);
extern gpio_mask_t PicoPin_Set(picopins_t* pins, gpio_mask_t new);

extern bool PicoPin_Reserve(picopins_t* pins, uint8_t pin);
extern void PicoPin_Release(picopins_t* pins, uint8_t pin);
extern bool PicoPin_isReserved(picopins_t* pins, uint8_t pin);

#endif  // PICOPINS_H

// EndFile: picopins.h
