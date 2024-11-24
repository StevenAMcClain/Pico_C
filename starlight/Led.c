// File: Led.c

#include "Common.h"

#include "hardware/clocks.h"

#include "led.h"
#include "ws2812.h"

//#define LED_Send(x) sw2812_Send((uint32_t*)(x))

// extern uint32_t disable_and_save_interrupts();			/* Used for interrupt disabling */
// extern void enable_and_restore_interrupts(uint32_t);	/* Used for interrupt enabling */
// extern void send_bit();

// #define LED_PIN 14    // The GPIO pin for the LED data.
// #define LED_PIN_MASK (1UL << LED_PIN)

PUBLIC size_t Num_LEDS = MAX_NUM_LEDS;

PUBLIC FLOAT LED_Brightness = 1.0;
 
/* Number of individual LEDs */
PUBLIC LED LED_Data[MAX_NUM_LEDS];
PRIVATE LED scaled_led_data[MAX_NUM_LEDS];

PRIVATE volatile bool do_update_leds = false;


// Sets a specific LED to a certain color.   LEDs start at 0
//
PUBLIC void LED_Set_RGB(size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
{
	if (led_idx < Num_LEDS)
	{
		LED* ledp = &LED_Data[led_idx];

		ledp->led.red   = r;	// Red.
		ledp->led.green = g;	// Green.
		ledp->led.blue  = b;	// Blue.
	}
}


PUBLIC void LED_Set_LED(size_t led_idx, LED* source_ledp)
{
	if (led_idx < Num_LEDS)
	{
		LED_Data[led_idx].val = source_ledp->val;
	}
}


// Sets all the LEDs to the same color.
//
PUBLIC void LED_All_LED(LED led)
{
	LED* ledp = LED_Data;
	size_t count = Num_LEDS;

	while (count--)
	{
		(ledp++)->val = led.val;
	}
}


// Sets all the LEDs to a certain color.
//
PUBLIC void LED_All_RGB(LED_VAL r, LED_VAL g, LED_VAL b)
{
	LED led;
	led.led.red   = r;
	led.led.green = g;
	led.led.blue  = b;

	LED_All_LED(led);
}


PRIVATE LED_VAL apply_brightness(LED_VAL val)
{
	uint32_t big_val = (uint32_t)(((FLOAT)val) * LED_Brightness);
	return (big_val <= MAX_LED_VAL) ? big_val : MAX_LED_VAL;
}


PRIVATE void scale_led_data()
{
	LED* bptr = LED_Data;
	LED* sptr = scaled_led_data;
	size_t bcount = Num_LEDS;

	while (bcount--)
	{
		sptr->led.red   = apply_brightness(bptr->led.red);
		sptr->led.green = apply_brightness(bptr->led.green);
		sptr->led.blue  = apply_brightness(bptr->led.blue);
		sptr++, bptr++;
	}
}


// Sends the data to the LEDs.
//
PUBLIC void LED_Do_Update(void)
{
	if (do_update_leds)
	{
		scale_led_data();
		sw2812_Send((uint32_t*)scaled_led_data);
		do_update_leds = false;
	}
}

// Sends the data to the LEDs.
//
PUBLIC void LED_Update(void)
{
	do_update_leds = true;
}

// Immediatly set all leds to black (off).
//
PUBLIC void LEDS_All_Black()
{
	LED_All_RGB(0, 0, 0);
	LED_Update();
}


// Call once at startup to setup leds.
//
PUBLIC void LED_Init(size_t num_leds)
{
	Num_LEDS = num_leds;

	sw2812_Init(num_leds);

	sleep_ms(10);	// Wait a bit to ensure clock is running and force LEDs to reset

	LEDS_All_Black();
}


// EndFile: Led.c
