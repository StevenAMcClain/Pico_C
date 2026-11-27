// File: debug_pin.h

#ifndef DEBUG_PIN_H
#define DEBUG_PIN_H

#include <stdbool.h>
#include <stdint.h>

extern void Debug_Pin_Initialize(uint8_t pin);

extern void Debug_Pin_Set(bool state);

#endif // DEBUG_PIN_H


// End of file: debug_pin.h
