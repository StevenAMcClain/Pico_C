// File: picopwm.c

#include "common.h"

#include <math.h>

// Output PWM signals on pins 0 and 1
#include <hardware/pwm.h>

const uint WRAP = 10000;

#define MAX_BRIGHT 1000

#define GAMMA 2.2

typedef struct 
{
    uint pin;
    uint slice_num;
    uint channel;
    uint duty;
    uint bright;

} PICO_PWM;


PUBLIC bool pico_pwm_initialize(PICO_PWM* pwm_pin, uint pin)
{
    if (pwm_pin)
    {
        pwm_pin->pin = pin;

        gpio_set_function(pin, GPIO_FUNC_PWM);

        pwm_pin->slice_num = pwm_gpio_to_slice_num(pin);
        pwm_pin->channel =  pwm_gpio_to_channel(pin);

        pwm_set_wrap(pwm_pin->slice_num, WRAP);

        pwm_set_chan_level(pwm_pin->slice_num, pwm_pin->channel, pwm_pin->duty);

        pwm_set_enabled(pwm_pin->slice_num, true);                      // Set the PWM running

        return true;
    }
    return false;
}


PRIVATE uint apply_gamma(uint x)
{
    double f = (double)x / (double)MAX_BRIGHT;
    double d = pow(f, GAMMA) * WRAP;
    return (int)d;
}

PRIVATE void pwm_set_duty(PICO_PWM* pwm_pin, uint bright)
{
    pwm_pin->bright = bright;
    pwm_pin->duty = apply_gamma(pwm_pin->bright);

    pwm_set_chan_level(pwm_pin->slice_num, pwm_pin->channel, pwm_pin->duty);
}


PUBLIC void pwm_test() 
{
    PICO_PWM pwm_pin;

    pico_pwm_initialize(&pwm_pin, 17);

    pwm_pin.bright = 1;

    int up = 1;

    while (1)
    {
        uint bright = pwm_pin.bright;
        if (up) { if (++bright == MAX_BRIGHT){ up = 0; } }
        else { if (--bright == 0) { up = 1; } }

        pwm_set_duty(&pwm_pin, bright);
        sleep_ms(1);		// Wait a bit to ensure clock is running and force LEDs to reset
    }
}


// EndFile: picopwm.c


#ifdef COMMENT
#include "common.h"

#include <hardware/flash.h>

#include <pico/flash.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include <math.h>
#include <stdio.h>

#include "blob.h"
#include "beng.h"
#include "debug.h"
#include "flashblob.h"
#include "led.h"
#include "parser.h"
#include "spi.h"
#include "spi_map.h"
#include "trace.h"

#include "morph.h"



PUBLIC uint32_t Debug_Mask = DEBUG_PRINTF; //DEBUG_ALL;


PUBLIC void _Printf(const char *fmt, ...) 
{
    if (Debug_Mask & DEBUG_PRINTF)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}


PUBLIC char* Get_UUID(void)
{
    static bool uuid_valid = false;
    static char uuid_string[] = "0000000000000000";

    if (!uuid_valid)
    {
        static uint8_t uuid[8] = {0};

        flash_get_unique_id(uuid);
        snprintf(uuid_string, sizeof(uuid_string), "%02X%02X%02X%02X%02X%02X%02X%02X",
                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7]);
        uuid_valid = true;
    }
    return uuid_string;
}


PRIVATE void Start_BlueTooth(void)
{
    extern void BlueTooth_Server(void);
    flash_safe_execute_core_init();
    BlueTooth_Server();
}


PUBLIC void main(void)
{
    stdio_init_all();   // Prepare stdio for use.
    pwm_main();
    while (1);

    // Trace_Init();
    char* uuid = Get_UUID();

    PRINTF("\r\n\nStarlight %s %s %s: UUID:%s: startup.\n", 
                            __VERSION__ , __TIME__, __DATE__, uuid);

    Spi_Init();         // Get SPI Port ready for use.

PRINTF("LED Morph size %d\n", sizeof(LED_MORPH));
PRINTF("LED Morph Single size %d\n", sizeof(LED_MORPH_SINGLE));
PRINTF("LED Morph Step size %d\n", sizeof(MORPH_STEP));
PRINTF("LED Morph Count size %d\n", sizeof(MORPH_COUNT));

    LED_Init();         // Prepare LED strings PIO driver for use.

    multicore_lockout_victim_init();
    multicore_launch_core1(Start_BlueTooth);   // Startup the Bluetooth stack.
    sleep_ms(100);      // Wait for BlueTooth to startup.
    
    Beng_Init();        // Get blob engines ready.
    Blob_Init();        // Get blob manager ready to run.

    Spi_Map_Init();     // Setup SPI device mapping.

    Start_Parser();     // Startup the parser.
}

#endif