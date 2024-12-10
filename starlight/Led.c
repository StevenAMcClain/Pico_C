// File: Led.c

#include "Common.h"

#include <stdlib.h>

#include "hardware/clocks.h"

#include "led.h"
#include "ws2812.h"

PUBLIC volatile uint32_t Current_PhyNum = 0;   // Current phynum (or logical).  0 for none.

PUBLIC FLOAT LED_Brightness = 1.0;			// Overall brightness for all strings.

PRIVATE volatile uint32_t needs_update = 0;     // Bit mask for strings that need to be updated.

typedef struct
{
    uint32_t mask;                  // Bit for phy mask.
	size_t led_count;				// Number of leds on string.
	LED* led_data;					// Data for LED string.
	LED* scaled_led_data;			// Buffer that is actually sent to LEDS.

} LEDS_PHY;


PRIVATE LEDS_PHY LEDS_Phy[MAX_PHY] = {0};


PUBLIC void PHY_Set_led_count(int phynum, size_t led_count)
//
// Set the number of leds on a string.  (re)Allocates buffers.
{
    if (phynum == 0)
        phynum = Current_PhyNum;

    LEDS_PHY* phy = LEDS_Phy;
    uint32_t mask = 1;
    int i = 0;

    while (phynum && i < MAX_PHY)
    {
        if (phynum & mask)
        {
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

                WS2812_Set_Num_LEDS(i, led_count);
            }
            phynum &= ~mask;
        }
        ++i;  ++phy;  mask <<= 1;
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
// Sends the data to the LEDs for all PHY that are flagged for update.
{
	LEDS_PHY* phy = LEDS_Phy;
	int i = 1;
    uint32_t mask = 1;

	while (needs_update && i <= MAX_PHY)
	{
		if (mask & needs_update)
		{
			scale_led_data(phy->led_data, phy->scaled_led_data, phy->led_count);
			WS2812_Prime_Send(mask, (uint32_t*)phy->scaled_led_data);
            needs_update &= ~mask;
		}
		++phy; ++i; mask <<= 1;
	}
	WS2812_Do_Send();   // Trigger DMA to start.... actuall send the data.
}

//------------------------------------------------ old

// PRIVATE LED LED_Data_One[MAX_NUM_LEDS];
// PRIVATE LED LED_Data_Two[MAX_NUM_LEDS];

//PUBLIC LED* LED_Data = LED_Data_One;

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


PUBLIC void LED_Needs_Update(int phynum)
//
// Sets the update flag(s) for phynum.
{
	if (phynum == 0)
		phynum = Current_PhyNum;

    needs_update |= phynum;
}


PRIVATE LED* get_ledp(int phyidx, size_t* num_ledsp)
{
	LED* result = NIL;

	if (phyidx >= 0 && phyidx < MAX_PHY)
    {
        LEDS_PHY* phy = LEDS_Phy + phyidx;

        result = phy->led_data;

        if (num_ledsp) { *num_ledsp = phy->led_count; }
    }
	return result;
}


PUBLIC size_t Num_LEDS(int phynum)
{
    int num_leds = 0;

	if (phynum == 0)								// Just current phy?
		phynum = Current_PhyNum;

	LEDS_PHY* phy = LEDS_Phy;
	int i = 0;

	while (phynum && i++ < MAX_PHY)
	{
        if (phynum & phy->mask)
        {
            if (phy->led_count > num_leds)
            {
                num_leds = phy->led_count;
            }
            phynum &= ~phy->mask;
        }
        ++phy;
    }

    return num_leds;
}


PRIVATE void do_LED_Set_RGB(int phyidx, size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
//
// Sets a specific LED to a certain color.   LEDs start at 0
{
	size_t num_leds = 0;
	LED* ledp_base = get_ledp(phyidx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;

		ledp->led.red   = r;	// Red.
		ledp->led.green = g;	// Green.
		ledp->led.blue  = b;	// Blue.

        needs_update |= (1 << phyidx);
	}
}


PRIVATE void do_LED_Set_LED(int phyidx, size_t led_idx, LED* source_ledp)
{
	size_t num_leds = 0;
	LED* ledp_base = get_ledp(phyidx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;
		ledp->val = source_ledp->val;

        needs_update |= (1 << phyidx);
	}
}


PUBLIC void LED_Set_LED(size_t led_idx, LED* source_ledp)
{
    int phynum = Current_PhyNum;

    uint32_t mask = 1;
    int i = 0;

    while (phynum && i < MAX_PHY)
    {
        if (phynum & mask)
        {
            do_LED_Set_LED(i, led_idx, source_ledp);
            phynum &= ~mask;
        }
        ++i;   mask <<= 1;
    }
}


PUBLIC void LED_Set_RGB(size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
    LED_Set_LED(led_idx, &led);
}


// Sets all LEDs to the same color.
//
PUBLIC void LED_All_LED(int phynum, LED led)
{
	if (phynum == 0)								// Just current phy?
		phynum = Current_PhyNum;

	LEDS_PHY* phy = LEDS_Phy;
	int i = 0;

	while (phynum && i++ < MAX_PHY)
	{
        if (phynum & phy->mask)
        {
			LED* ledp = phy->led_data;
			size_t count = phy->led_count;

			while (count--)
			{
				(ledp++)->val = led.val;
			}

            needs_update |= phy->mask;
            phynum &= ~phy->mask;
        }
        ++phy;
    }
}


// Sets all the LEDs to a certain color.
//
PUBLIC void LED_All_RGB(int phynum, LED_VAL r, LED_VAL g, LED_VAL b)
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
	LED_All_LED(phynum, led);
}


// Immediately set ALL leds to black (off).
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
    LEDS_PHY* phy = LEDS_Phy;
    int i = MAX_PHY;
    uint32_t mask = 1;

    while (i--)
    {
        phy->mask = mask;
        ++phy;  mask <<= 1;
    }

	WS2812_Init();		// Starts LED driver for all PHYs.

	sleep_ms(10);		// Wait a bit to ensure clock is running and force LEDs to reset

	LEDS_All_Black();
}


// EndFile: Led.c
