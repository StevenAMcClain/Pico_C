// File: Led.c

#include "common.h"

// #include <stdlib.h>

#include <hardware/clocks.h>

#include "led.h"
#include "ws2812.h"


PUBLIC FLOAT LED_Brightness = 1.0;			    // Overall brightness for all strings.

PRIVATE volatile uint32_t needs_update = 0;     // Bit mask for strings that need to be updated.

typedef struct
{
	size_t led_count;				// Number of leds on string.
	LED* led_data;					// Data for LED string.
	LED* scaled_led_data;			// Buffer that is actually sent to LEDS.

} LEDS_PHY;

PRIVATE LED Leds_Buff[MAX_NUM_LEDS];   // All LED values in the entire system are stored here!
PRIVATE size_t Leds_Allocated = 0;

PRIVATE LEDS_PHY LEDS_Phy[MAX_PHY] = {0};


PUBLIC void LEDS_Buff_Reset()
{
    Leds_Allocated = 0;

    LEDS_PHY* phy = LEDS_Phy;
    int i = 0;

    while (i < MAX_PHY)
    {
        phy->led_count = 0;
        phy->led_data = phy->scaled_led_data = 0;
        ++phy; ++i;
    }
}


PUBLIC LED* LEDS_Buff_Allocate(size_t size)
{
    LED* leds = 0;

    if (Leds_Allocated + size < MAX_NUM_LEDS)
    {
        leds = &Leds_Buff[Leds_Allocated];
        Leds_Allocated += size;
    }
    return leds;
}


PUBLIC size_t PHY_Get_LED_Count(int phy_idx)
{
	if (PHY_IDX_VALID(phy_idx))
    {
        LEDS_PHY* phy = LEDS_Phy + phy_idx;
        return phy->led_count;
    }
    return (-1);
}


PUBLIC void PHY_Set_led_count(int phy_idx, size_t led_count)
//
// Set the number of leds on a string.  (re)Allocates buffers.
{
    if (PHY_IDX_VALID(phy_idx) && led_count > 0)
    {
        LEDS_PHY* phy = LEDS_Phy + phy_idx;

        LED* buff = LEDS_Buff_Allocate(2 * led_count);   // Allocate room for LED values and scaled LED values.

        if (buff)
        {
            phy->led_data = buff;
            phy->scaled_led_data = buff + led_count;
            phy->led_count = led_count;
        }
        else 
        {
            phy->led_data = phy->scaled_led_data = NIL;
            phy->led_count = 0; 
        }

        WS2812_Set_Num_LEDS(phy_idx, led_count);
    }
}


// PUBLIC void LEDS_Set_Phynum(int phy_mask)
// {
// 	Current_Phy_Mask = phy_mask;
// }


PRIVATE inline LED_VAL apply_brightness(LED_VAL val)
{
	uint32_t big_val = (uint32_t)(((FLOAT)val) * LED_Brightness);
	return (big_val <= LED_VAL_MAX) ? big_val : LED_VAL_MAX;
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
    int idx = 1;
    uint32_t mask = 1;

    while (needs_update && idx <= MAX_PHY)
    {
        if (mask & needs_update)
        {
            scale_led_data(phy->led_data, phy->scaled_led_data, phy->led_count);
            WS2812_Prime_Send(mask, (uint32_t*)phy->scaled_led_data);
            needs_update &= ~mask;
        }
        ++phy; ++idx; mask <<= 1;
    }
    WS2812_Do_Send();   // Trigger DMA to start.... actually send the data.
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


PUBLIC void LED_Needs_Update(int phy_mask)
//
// Sets the update flag(s) for phynum.
{
    needs_update |= phy_mask;
}


PUBLIC LED* LED_Get_Phy(int phy_idx, size_t* num_ledsp)
{
	LED* result = NIL;

	if (PHY_IDX_VALID(phy_idx))
    {
        LEDS_PHY* phy = LEDS_Phy + phy_idx;

        result = phy->led_data;

        if (num_ledsp) { *num_ledsp = phy->led_count; }
    }
	return result;
}


PUBLIC size_t Num_LEDS(int phy_mask)
{
    int num_leds = 0;

//	if (phy_mask == 0)								// Just current phy?
//		phy_mask = Current_Phy_Mask;

	LEDS_PHY* phy = LEDS_Phy;
	int idx = 0;
    uint32_t mask = 1;

	while (phy_mask && idx++ < MAX_PHY)
	{
        if (mask & phy_mask)
        {
            if (phy->led_count > num_leds)
            {
                num_leds = phy->led_count;
            }
            phy_mask &= ~mask;
        }
        ++phy;  mask <<= 1;
    }

    return num_leds;
}


PRIVATE void do_LED_Set_RGB(int phy_idx, size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
//
// Sets a specific LED to a certain color.   LEDs start at 0
{
	size_t num_leds = 0;
	LED* ledp_base = LED_Get_Phy(phy_idx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;

		ledp->led.red   = r;	// Red.
		ledp->led.green = g;	// Green.
		ledp->led.blue  = b;	// Blue.

        needs_update |= (1 << phy_idx);
	}
}


PRIVATE void do_LED_Set_LED(int phy_idx, size_t led_idx, LED* source_ledp)
{
	size_t num_leds = 0;
	LED* ledp_base = LED_Get_Phy(phy_idx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;
		ledp->val = source_ledp->val;

        needs_update |= (1 << phy_idx);
	}
}


PUBLIC void LED_Set_LED(int phy_mask, size_t led_idx, LED* source_ledp)
{
    uint32_t mask = 1;
    int idx = 0;

    while (phy_mask && idx < MAX_PHY)
    {
        if (phy_mask & mask)
        {
            do_LED_Set_LED(idx, led_idx, source_ledp);
            phy_mask &= ~mask;
        }
        ++idx;   mask <<= 1;
    }
}


PUBLIC void LED_Set_RGB(int phy_mask, size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
    LED_Set_LED(phy_mask, led_idx, &led);
}


PUBLIC void LED_All_LED(int phy_mask, LED led)
//
// Sets all LEDs to the same color.
{
	LEDS_PHY* phy = LEDS_Phy;
	int idx = 0;
    uint32_t mask = 1;

	while (phy_mask && idx++ < MAX_PHY)
	{
        if (mask & phy_mask)
        {
			LED* ledp = phy->led_data;
			size_t count = phy->led_count;

			while (count--)
			{
				(ledp++)->val = led.val;
			}

            needs_update |= mask;
            phy_mask &= ~mask;
        }
        ++phy; mask <<= 1;
    }
}


PUBLIC void LED_All_RGB(int phy_mask, LED_VAL r, LED_VAL g, LED_VAL b)
//
// Sets all the LEDs to a certain color.
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
	LED_All_LED(phy_mask, led);
}


PUBLIC void LEDS_All_Black()
//
// Immediately set ALL leds to black (off).
{
	LED_All_RGB(ALL_PHYS, 0, 0, 0);
	LEDS_Do_Update();			// Write all LEDs NOW!
}


PUBLIC void LED_Init(void)
//
// Call once at startup to setup leds.
{
	WS2812_Init();		// Starts LED driver for all PHYs.
	sleep_ms(10);		// Wait a bit to ensure clock is running and force LEDs to reset
	LEDS_All_Black();   // Start with everything off.
}


// EndFile: Led.c
