// File: Led.c

#include "common.h"

// #include <stdlib.h>

#include <hardware/clocks.h>

#include "led.h"
#include "ws2812.h"

#include "morph.h"

//#define TRACE_LED_BRIGHTNESS

#ifdef TRACE_LED_BRIGHTNESS
#include "trace.h"
#endif


PUBLIC uint32_t LED_Brightness = 1024;			// Overall brightness for all strings (0-1024 == 0-100%).

PRIVATE volatile uint32_t needs_update = 0;     // Bit mask for strings that need to be updated.

typedef struct
{
	size_t led_count;				// Number of leds on string.
	LED* led_data;					// Data for LED string.
	LED* scaled_led_data;			// Buffer that is actually sent to LEDS.
    LED_MORPH morph_data;           // Data used while morphing.

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
        phy->led_data = phy->scaled_led_data = NIL;
        phy->morph_data.dests = NIL;
        ++phy; ++i;
    }
}


PUBLIC LED* LEDS_Buff_Allocate(size_t size)
{
    LED* leds = NIL;

    if (Leds_Allocated + size < MAX_NUM_LEDS)
    {
        leds = &Leds_Buff[Leds_Allocated];
        Leds_Allocated += size;
    }
    return leds;
}


PUBLIC size_t LEDS_Buff_Available(void)
{
    return MAX_NUM_LEDS - Leds_Allocated;
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

//#define LED_MULTIPLIER 2   // Only led_data and scaled_led_data.
#define LED_MULTIPLIER 6   // led_data and scaled_led_data and morph data.


PUBLIC void PHY_Set_led_count(int phy_idx, size_t led_count)
//
// Set the number of leds on a string.  (re)Allocates buffers.
{
    if (PHY_IDX_VALID(phy_idx) && led_count > 0)
    {
        LEDS_PHY* phy = LEDS_Phy + phy_idx;

        LED* buff = LEDS_Buff_Allocate(led_count * LED_MULTIPLIER);   // Allocate room for LED values and scaled LED values.

        if (buff)
        {
            phy->led_data =                             (buff + (0 * led_count));
            phy->scaled_led_data =                      (buff + (1 * led_count));  // Right after led_data
            phy->morph_data.dests =                     (buff + (2 * led_count));  // After led_data and scaled_led_data
            phy->morph_data.morphs = (LED_MORPH_SINGLE*)(buff + (3 * led_count));  // After led_data and scaled_led_data
            phy->led_count = led_count;

            WS2812_Set_Num_LEDS(phy_idx, led_count,  (uint32_t*)phy->scaled_led_data);
        }
        else 
        {
            phy->led_data = phy->scaled_led_data = NIL;
            phy->morph_data.dests = NIL;
            phy->morph_data.morphs = NIL;
            phy->led_count = 0; 
        }
    }
}


PRIVATE inline LED_VAL apply_brightness(LED_VAL val)
{
	uint32_t big_val = (val * LED_Brightness) >> 10;
	return (big_val <= LED_VAL_MAX) ? big_val : LED_VAL_MAX;
}


PRIVATE inline void scale_led_data(LED* bptr, LED* sptr, size_t bcount)
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
    int phy_idx = 0;
    uint32_t phy_mask = 1;

#ifdef TRACE_LED_BRIGHTNESS
    Trace_Start();
#endif

    while (needs_update && phy_idx < MAX_PHY)
    {
        if (phy_mask & needs_update)
        {
            scale_led_data(phy->led_data, phy->scaled_led_data, phy->led_count);
            WS2812_Set_Primed(phy_mask);
            needs_update &= ~phy_mask;
        }
        ++phy; ++phy_idx; phy_mask <<= 1;
    }

#ifdef TRACE_LED_BRIGHTNESS
    Trace_End();
#endif
}


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


PUBLIC size_t Num_LEDS_Mask(int phy_mask)
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


PRIVATE void LED_Set_RGB_Idx(int phy_idx, size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
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


PRIVATE void LED_Set_LED_Idx(int phy_idx, size_t led_idx, LED* source_ledp)
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


PUBLIC void LED_Set_LED_Mask(int phy_mask, size_t led_idx, LED* source_ledp)
{
    uint32_t mask = 1;
    int idx = 0;

    while (phy_mask && idx < MAX_PHY)
    {
        if (phy_mask & mask)
        {
            LED_Set_LED_Idx(idx, led_idx, source_ledp);
            phy_mask &= ~mask;
        }
        ++idx;   mask <<= 1;
    }
}


PUBLIC void LED_Set_RGB_Mask(int phy_mask, size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b)
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
    LED_Set_LED_Mask(phy_mask, led_idx, &led);
}
  

PUBLIC void LED_All_LED_Mask(int phy_mask, LED led)
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


PUBLIC void LED_All_RGB_Mask(int phy_mask, LED_VAL r, LED_VAL g, LED_VAL b)
//
// Sets all the LEDs to a certain color.
{
    LED led = {.led.red = r, .led.green = g, .led.blue = b};
	LED_All_LED_Mask(phy_mask, led);
}


PUBLIC void LEDS_All_Black(void)
//
// Immediately set ALL leds to black (off).
{
	LED_All_RGB_Mask(ALL_PHYS, 0, 0, 0);
	LEDS_Do_Update();			// Write all LEDs NOW!
}


PUBLIC void LED_Init(void)
//
// Call once at startup to setup leds.
{
	WS2812_Init();		// Starts LED driver for all PHYs.
	LEDS_All_Black();   // Start with everything off.
}


// EndFile: Led.c
