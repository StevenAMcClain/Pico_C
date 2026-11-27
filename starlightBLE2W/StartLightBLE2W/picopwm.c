// File: picopwm.c

#include "common.h"
#include "picopwm.h"

#include <stdio.h>
#include <math.h>

#include <hardware/pwm.h>
#include <pico/stdlib.h>

#include "error.h"
#include "picopins.h"


const uint32_t WRAP = 10000;    // Controls PWM frequency.

#define GAMMA 2.2


typedef struct 
{
    bool use_gamma;

    uint32_t pin;
    uint32_t slice_num;
    uint32_t channel;
    uint32_t duty;
    uint32_t bright;

} PICO_PWM;

PRIVATE PICO_PWM pwm_pins[MAX_PWM_PIN] = {0};

PRIVATE void* PicoPWM_Get_Record(uint8_t pin)
{
    if (pin < MAX_PWM_PIN)
    {
        return &pwm_pins[pin];
    }
    else { POST_ERROR("Can't %d!\n", pin); }

    return NULL;
}


PRIVATE uint32_t apply_gamma(uint32_t x)
{
    double v = (double)x;
    double f = v / (double)MAX_BRIGHT;
    double d = pow(f, GAMMA) * WRAP;

    return (uint32_t)d;
}


PUBLIC bool PicoPWM_Initialize(uint8_t pin, bool use_gamma)
{
    PICO_PWM* pwm = PicoPWM_Get_Record(pin);

    if (pwm)
    {
        pwm->pin = pin;
        pwm->use_gamma = use_gamma;

        gpio_set_function(pin, GPIO_FUNC_PWM);

        pwm->channel =   pwm_gpio_to_channel(pin);
        pwm->slice_num = pwm_gpio_to_slice_num(pin);

        pwm_set_wrap(pwm->slice_num, WRAP);

        pwm_set_chan_level(pwm->slice_num, pwm->channel, pwm->duty);

        pwm_set_enabled(pwm->slice_num, true);                      // Set the PWM running

        return true;
    }
    else { POST_ERROR("can't  get pwm %d!", pin); }

    return false;
}


PUBLIC void PicoPWM_Set_Bright(uint8_t pin, uint32_t bright)
{
    PICO_PWM* pwm = PicoPWM_Get_Record(pin);

    if (pwm)
    {
        if (bright > MAX_BRIGHT) { bright = MAX_BRIGHT; }

        pwm->bright = bright;

        pwm->duty = pwm->use_gamma ? apply_gamma(bright) 
                                   : (bright * WRAP) / MAX_BRIGHT;

        pwm_set_chan_level(pwm->slice_num, pwm->channel, pwm->duty);
    }
    else { POST_ERROR("can't  get pwm %d!", pin); }
}


PUBLIC uint32_t PicoPWM_Get_Bright(uint8_t pin)
{
    PICO_PWM* pwm = PicoPWM_Get_Record(pin);

    if (!pwm) { POST_ERROR("can't  get pwm %d!", pin); }

    return pwm ? pwm->bright : 0;
 }


// Endfile: picopwm.c