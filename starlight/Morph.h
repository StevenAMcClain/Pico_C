// File: Morph.h

#ifndef MORPH_H
#define MOROH_H

#include "led.h"

// typedef struct morph
// {
//     uint8_t dest:8;      // Destination value.
//      int8_t step:8;      // How large of a step each time.
//     uint8_t count:8;     // Countdown until next step.
//     uint8_t period:8;    // Number of ticks between each step.

// } MORPH;

// typedef struct led_morph
// {
//     MORPH wbrg[4];  // one for each color.

// } LED_MORPH;

typedef struct morph_step
{
    int8_t white;
    int8_t blue;
    int8_t red;
    int8_t green;

} MORPH_STEP;

typedef struct morph_count
{
    uint8_t white;
    uint8_t blue;
    uint8_t red;
    uint8_t green;

} MORPH_COUNT;

typedef struct led_morph_single
{
    MORPH_STEP  steps;
    MORPH_COUNT counts;
    MORPH_COUNT periods;

} LED_MORPH_SINGLE;

typedef struct led_morph
{
    size_t num_leds;
    LED* dests;
    LED_MORPH_SINGLE* morphs;

} LED_MORPH;

// extern void morph_start(FLOAT trans_time, LED* to_scene);
// extern bool morph_step(void);

#endif // MORPH_H

// Endfile: Morph.h
