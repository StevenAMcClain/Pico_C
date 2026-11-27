// File: picopwm.h

#ifndef PICOPWM_H
#define PICOPWM_H

#define MAX_PWM_PIN 32

#define MAX_BRIGHT 1000     // 100.0%

#define USE_GAMMA true
#define USE_LINEAR false

extern bool PicoPWM_Initialize(uint8_t /*pin*/, bool /*use_gamma*/);
extern void PicoPWM_Set_Bright(uint8_t /*pin*/, uint32_t /*bright*/);
extern uint32_t PicoPWM_Get_Bright(uint8_t /*pin*/);

#endif  // PICOPWM_H


// EndFile: picopwm.h
