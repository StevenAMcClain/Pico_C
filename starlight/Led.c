// File: Led.c

#include "Common.h"

#include <stdlib.h>

#include "hardware/clocks.h"

#include "led.h"
#include "ws2812.h"

PUBLIC FLOAT LED_Brightness = 1.0;			// Overall brighness for all strings.


typedef struct
{
	size_t led_count;				// Number of leds on string.
	LED* led_data;					// Data for LED string.
	LED* scaled_led_data;			// Buffer that is actually sent to LEDS.
	volatile bool needs_update;		// true to trigger update.

} LEDS_PHY;


PRIVATE LEDS_PHY LEDS_Phy[MAX_PHY] = {0};


PUBLIC void PHY_Set_led_count(int phynum, size_t led_count)
//
// Set the number of leds on a string.  (re)Allocates buffers.
{
	if (phynum == 0)
	{
		phynum = Current_PhyNum;
	}

	if (phynum < 0)
	{
        phynum = 1;
        while (phynum <= MAX_PHY)
        {
            PHY_Set_led_count(phynum, led_count);      // Recurse.
            ++phynum;
        }
	}
	else if (phynum > 0 && phynum < MAX_PHY)
	{
		LEDS_PHY* phy = LEDS_Phy + phynum - 1;

		if (led_count != phy->led_count)		// Is this a different led_count?
		{
			if (phy->led_data)		// Is there already a data buffer?
			{
				free(phy->led_data);						// Yes, free buffer.
				phy->led_data = phy->scaled_led_data = 0;   // And zero out pointers.
			}

			if (led_count > 0)
			{
				phy->led_data = malloc(led_count * 2 * LED_SIZE);                                // ---------  Malloc!
				if (phy->led_data)
				{
					phy->scaled_led_data = phy->led_data + led_count * LED_SIZE;
					phy->led_count = led_count;
				}
				else { phy->led_count = 0; }
			}
			else { phy->led_count = 0; }

			WS2812_Set_Num_LEDS(phynum, led_count);
		}
	}
}


PUBLIC void LEDS_Set_Phynum(int phynum)
{
	Current_PhyNum = phynum;
}


PRIVATE LED_VAL apply_brightness(LED_VAL val)
{
	uint32_t big_val = (uint32_t)(((FLOAT)val) * LED_Brightness);
	return (big_val <= MAX_LED_VAL) ? big_val : MAX_LED_VAL;
}


PRIVATE void scale_led_data(LED* bptr, LED* sptr, size_t bcount)
{
	while (bcount--)
	{
		sptr->led.red   = apply_brightness(bptr->led.red);
		sptr->led.green = apply_brightness(bptr->led.green);
		sptr->led.blue  = apply_brightness(bptr->led.blue);
		sptr++, bptr++;
	}
}


PUBLIC void LEDS_Do_Update(void)
//
// Sends the data to the LEDs for all PHY.
{
	LEDS_PHY* phy = LEDS_Phy;
	int i = 1;

	while (i <= MAX_PHY)
	{
		if (phy->needs_update)
		{
			scale_led_data(phy->led_data, phy->scaled_led_data, phy->led_count);
			WS2812_Send(i, (uint32_t*)phy->scaled_led_data);
			phy->needs_update = false;
		}
		++phy; ++i;
	}
}

//------------------------------------------------ old

// PRIVATE volatile bool do_update_leds = false;  

//PUBLIC size_t Num_LEDS = MAX_NUM_LEDS;

 
// PRIVATE LED LED_Data_One[MAX_NUM_LEDS];
// PRIVATE LED LED_Data_Two[MAX_NUM_LEDS];

//PUBLIC LED* LED_Data = LED_Data_One;

//PRIVATE LED scaled_led_data[MAX_NUM_LEDS];    // This is the buffer that is actually sent to LEDS.


// PUBLIC LED* ALT_LED_Data(void)
// //
// //  Returns an alternate LED_Buffer.
// {
// 	return LED_Data == LED_Data_One ? LED_Data_Two : LED_Data_One;
// }

// PUBLIC void Switch_ALT_LED_Data(void)
// //
// //  Sets alternate buffer to current.
// {
// 	LED_Data = ALT_LED_Data();
// 	LED_Update(0);
// }


// Sends the data to the LEDs.
//
PUBLIC void LED_Update(int phynum)
{
	if (phynum == 0)
		phynum = Current_PhyNum;

	if (phynum < 0 )
	{
        phynum = 1;
        while (phynum <= MAX_PHY)
        {
            LED_Update(phynum);      // Recurse.
            ++phynum;
        }
	}
	else if (phynum > 0)
	{
		if (phynum <= MAX_PHY)
		{
			LEDS_PHY* phy = &LEDS_Phy[phynum - 1];
			phy->needs_update = true;
		}
		else											// Logical (virtual?) phy?
		{
			// > MAX_PHY is logical string.
		}
	}
}


PRIVATE LED* get_ledp(size_t* num_ledsp)
{
	LED* result = NIL;

	if (Current_PhyNum > 0)
	{
		if (Current_PhyNum <= MAX_PHY)
		{
			LEDS_PHY* phy = LEDS_Phy + Current_PhyNum - 1;

			result = phy->led_data;

			if (num_ledsp)
			{
				*num_ledsp = phy->led_count;
			}
		}
	}
	return result;
}


PUBLIC size_t Num_LEDS(void)
{
	size_t num_leds = 0;
	(void)get_ledp(&num_leds);
	return num_leds;
}


PUBLIC void LED_Set_RGB(size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
//
// Sets a specific LED to a certain color.   LEDs start at 0
{
	size_t num_leds = 0;
	LED* ledp_base = get_ledp(&num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;

		ledp->led.red   = r;	// Red.
		ledp->led.green = g;	// Green.
		ledp->led.blue  = b;	// Blue.
	}
}


PUBLIC void LED_Set_LED(size_t led_idx, LED* source_ledp)
{
	size_t num_leds = 0;
	LED* ledp_base = get_ledp(&num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;
		ledp->val = source_ledp->val;
	}
}


// Sets all the LEDs to the same color.
//
PUBLIC void LED_All_LED(int phynum, LED led)
{
	if (phynum == 0)								// Just current phy?
		phynum = Current_PhyNum;

	if (phynum < 0)									// ALL phys?
	{
		phynum = 1;
		while (phynum <= MAX_PHY)
		{
			LED_All_LED(phynum, led);
            ++phynum;
		}
	}
	else if (phynum > 0)
	{
		if (phynum <= MAX_PHY)			// Specific phy?
		{
			LEDS_PHY* phy = &LEDS_Phy[phynum - 1];
			LED* ledp = phy->led_data;
			size_t count = phy->led_count;

			while (count--)
			{
				(ledp++)->val = led.val;
			}

			phy->needs_update = true;
		}
		else											// Logical (virtual?) phy?
		{
			// > MAX_PHY is logical string.
		}
	}
}


// Sets all the LEDs to a certain color.
//
PUBLIC void LED_All_RGB(int phynum, LED_VAL r, LED_VAL g, LED_VAL b)
{
	LED led;
	led.led.red   = r;
	led.led.green = g;
	led.led.blue  = b;

	LED_All_LED(phynum, led);
}


// Immediatly set ALL leds to black (off).
//
PUBLIC void LEDS_All_Black()
{
	LED_All_RGB(ALL_PHYS, 0, 0, 0);
	LEDS_Do_Update();			// Write all LEDs NOW!
}


// Call once at startup to setup leds.
//
PUBLIC void LED_Init(void)
{
//	Num_LEDS = num_leds;

	WS2812_Init();		// Starts LED driver for all PHYs.

	sleep_ms(10);		// Wait a bit to ensure clock is running and force LEDs to reset

	LEDS_All_Black();
}


// EndFile: Led.c
