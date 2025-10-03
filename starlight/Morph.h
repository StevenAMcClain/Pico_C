// File: Morph.h

#ifndef MORPH_H
#define MOROH_H

typedef struct morph
{
    int32_t step;
    uint32_t time;
    uint32_t period;

} MORPH;

typedef struct led_morph
{
    MORPH wbrg[4];  // one for each color.

} LED_MORPH;

// extern void morph_start(FLOAT trans_time, LED* to_scene);
// extern bool morph_step(void);

#endif // MORPH_H

// Endfile: Morph.h
