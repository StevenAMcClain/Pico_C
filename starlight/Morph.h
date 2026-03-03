// File: Morph.h

#ifndef MORPH_H
#define MORPH_H

#include "blob.h"
#include "led.h"

// LED Morph size 12                                                               
// LED Morph Single size 12                                                        
// LED Morph Step size 4                                                           
// LED Morph Count size 4        

typedef struct morph_step
{
    int8_t x;
    int8_t blue;
    int8_t red;
    int8_t green;

} MORPH_STEP;

typedef struct morph_count
{
    uint8_t x;
    uint8_t blue;
    uint8_t red;
    uint8_t green;

} MORPH_COUNT;

typedef struct led_morph_single
{
    MORPH_STEP  steps;          // How much for each step.
    MORPH_COUNT counts;         // Current count until next step.
    MORPH_COUNT periods;        // Count for one step.

} LED_MORPH_SINGLE;

typedef struct led_morph
{
    size_t num_leds;            // Number of leds on this string.
    LED* dests;                 // Destination values.
    LED_MORPH_SINGLE* morphs;   // Morph records for each value of each led.

} LED_MORPH;

extern void Morph_Start(LED* /*start_leds*/, LED_MORPH*, int /*morph_time*/, int /*tick_speed*/, SCENE_ID /*new_scene*/);

extern bool Morph_Step(void* /*bs*/);

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

// typedef struct morph_step
// {
//     int8_t white;
//     int8_t blue;
//     int8_t red;
//     int8_t green;

// } MORPH_STEP;

// typedef struct morph_count
// {
//     uint8_t white;
//     uint8_t blue;
//     uint8_t red;
//     uint8_t green;

// } MORPH_COUNT;

// typedef struct led_morph_single
// {
//     MORPH_STEP  steps;
//     MORPH_COUNT counts;
//     MORPH_COUNT periods;

// } LED_MORPH_SINGLE;

// typedef struct led_morph
// {
//     size_t num_leds;
//     LED* dests;
//     LED_MORPH_SINGLE* morphs;

// } LED_MORPH;

// extern void morph_start(FLOAT trans_time, LED* to_scene);
// extern bool morph_step(void);

#endif // MORPH_H

// Endfile: Morph.h
