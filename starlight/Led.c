// File: Led.c

#include "common.h"

#include <string.h>

#include <hardware/clocks.h>

#include "led.h"
#include "ws2812.h"


//#define TRACE_LED_BRIGHTNESS

#ifdef TRACE_LED_BRIGHTNESS
#include "trace.h"
#endif


//PUBLIC uint32_t LED_Brightness = 1024;			// Overall brightness for all strings (0-1024 == 0-100%).
PUBLIC uint32_t LED_Brightness = 6;			// Overall brightness for all strings (0-1024 == 0-100%).

PRIVATE volatile uint32_t Needs_Update_Mask = 0;     // Bit mask for strings that need to be updated.

PRIVATE LED Leds_Buff[MAX_NUM_LEDS];   // All LED values in the entire system are stored here!
PRIVATE size_t Leds_Allocated = 0;

PRIVATE LEDS_PHY LEDS_Phy[MAX_PHY] = {0};


PRIVATE void LEDS_Buff_Reset()
//
// Clear all phystrings and remove all mirrors.
// Clear all led pool allocations.
{
    LEDS_PHY* phy = LEDS_Phy;
    int n = MAX_PHY;

    while (n--)
    {
        phy->led_count = 0;
        phy->led_data = phy->scaled_led_data = NIL;
        ++phy;
    }

    Leds_Allocated = 0;
}


PUBLIC LED* LEDS_Buff_Allocate(size_t size)
//
// Allocate leds from led pool.
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
//
// Returns the number of leds available in pool.
{
    return MAX_NUM_LEDS - Leds_Allocated;
}


PUBLIC size_t PHY_Get_LED_Count(int phy_idx)
//
// Get the number of leds on a specific phy string.
// Return (-1) if phy is not used.
{
	if (PHY_IDX_VALID(phy_idx))
    {
        LEDS_PHY* phy = LEDS_Phy + phy_idx;
        return phy->led_count;
    }
    return (-1);
}


PRIVATE uint32_t build_mirror_mask(int idx)
{
    uint32_t result = (1 << idx++);
    LEDS_PHY* phy = LEDS_Phy;
    uint32_t mask = 1;
    int n = MAX_PHY;

    while (n--)
    {
        if (idx == phy->mirroring) { result |= mask; }
        mask <<= 1; ++phy;
    }

    return ~result;
}


PUBLIC void PHY_Build_Mirror_Masks(void)
//
// Called after setting up phy table to build the mirror masks.
{
    LEDS_PHY* phy = LEDS_Phy;
    int idx = 0;

    while (idx < MAX_PHY)
    {
        phy->mirror_mask = build_mirror_mask(idx);
        ++idx; ++phy;
    }

    phy = LEDS_Phy;
    int n = MAX_PHY;

    while (n--)
    {
        if (phy->mirroring)
        {
            LEDS_PHY* phyx = LEDS_Phy + phy->mirroring - 1;
            phy->mirror_mask = phyx->mirror_mask; 
        }

        ++phy;
    }
}


#define LED_MULTIPLIER 2   // Only led_data and scaled_led_data.


PUBLIC void PHY_Set_Led_Count(int val)
//
// Set the number of leds on a string.  Allocates buffers if not mirrored.
{
    int idx = ((val >> 24) - 1) & 0xF;
    int mir = (val >> 16) & 0xF;
    int num =  val & 0xFFFF;

    if (PHY_IDX_VALID(idx))
    {
        LEDS_PHY* phy = LEDS_Phy + idx;

        if (num > 0)
        {
            LED* buff = LEDS_Buff_Allocate(num * LED_MULTIPLIER);   // Allocate room for LED values and scaled LED values.

            if (buff)
            {
                phy->led_data =                             (buff + (0 * num));
                phy->scaled_led_data =                      (buff + (1 * num));  // Right after led_data
                phy->led_count = num;
                phy->mirroring = mir;

                // Set LED records to have phy index.
                //
                LED* ledp = phy->led_data;
                int n = num;

                while (n--)
                {
                    ledp->led.phy_num = idx + 1;
                    ++ledp;
                }

                WS2812_Set_Num_LEDS(idx, num,  (uint32_t*)phy->scaled_led_data);
            }
        }
        else
        {
            if (PHY_IDX_VALID(mir))
            {
                LEDS_PHY* mir_phy = LEDS_Phy + mir - 1;

                phy->led_data          = mir_phy->led_data;
                phy->scaled_led_data   = mir_phy->scaled_led_data;
                phy->led_count         = mir_phy->led_count; 
                phy->mirroring         = mir;

                WS2812_Set_Num_LEDS(idx, phy->led_count, (uint32_t*)phy->scaled_led_data);
            }
            else
            {
                phy->led_data = phy->scaled_led_data = NIL;
                phy->mirroring = phy->led_count = 0; 
            }
        }
    }
}



//#define LED_MULTIPLIER 6   // led_data and scaled_led_data and morph data.
                // phy->morph_data.dests =                     (buff + (2 * num));  // After led_data and scaled_led_data
                // phy->morph_data.morphs = (LED_MORPH_SINGLE*)(buff + (3 * num));  // After led_data and scaled_led_data

// ---------------------------------------------------------------------------

PRIVATE inline LED_VAL apply_brightness(LED_VAL val)
//
// return scaled led value using brightness.
{
	uint32_t big_val = (val * LED_Brightness) >> 10;
	return (big_val <= LED_VAL_MAX) ? big_val : LED_VAL_MAX;
}


PRIVATE inline void scale_led_data(LED* bptr, LED* sptr, size_t bcount)
//
// scale each value in each LED record of phy.
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
    uint32_t phy_mask = 1;
    int phy_idx = 0;

#ifdef TRACE_LED_BRIGHTNESS
    Trace_Start();
#endif
    while (Needs_Update_Mask && phy_idx < MAX_PHY)
    {
        if (phy_mask & Needs_Update_Mask)
        {
            scale_led_data(phy->led_data, phy->scaled_led_data, phy->led_count);
            WS2812_Set_Primed(~phy->mirror_mask);
            Needs_Update_Mask &= phy->mirror_mask;
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
    Needs_Update_Mask |= phy_mask;
}


PUBLIC LEDS_PHY* LED_Get_Phy(int phy_idx)
{
    return PHY_IDX_VALID(phy_idx) ? LEDS_Phy + phy_idx : NULL;
}


PUBLIC LED* LED_Get_LED_Data(int phy_idx, size_t* num_ledsp)
//
// Get pointer to LED records and led_count for specific phy.
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
//
// Returns the largest led_count for a number of phys.
{
    int num_leds = 0;

	LEDS_PHY* phy = LEDS_Phy;
    uint32_t mask = 1;
	int n = MAX_PHY;

	while (phy_mask && n--)
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
	LED* ledp_base = LED_Get_LED_Data(phy_idx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;

		ledp->led.red   = r;	// Red.
		ledp->led.green = g;	// Green.
		ledp->led.blue  = b;	// Blue.

        Needs_Update_Mask |= (1 << phy_idx);
	}
}


PRIVATE void LED_Set_LED_Idx(int phy_idx, size_t led_idx, LED* source_ledp)
{
	size_t num_leds = 0;
	LED* ledp_base = LED_Get_LED_Data(phy_idx, &num_leds);

	if (ledp_base && led_idx < num_leds)
	{
		LED* ledp = ledp_base + led_idx;

		ledp->val = source_ledp->val;

        Needs_Update_Mask |= (1 << phy_idx);
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
    uint32_t mask = 1;
	int n = MAX_PHY;

	while (phy_mask && n--)
	{
        if (mask & phy_mask)
        {
			LED* ledp = phy->led_data;
			size_t count = phy->led_count;

			while (count--)
			{
				(ledp++)->val = led.val;
			}

            Needs_Update_Mask |= mask;
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


PUBLIC void LEDS_Phy_Reset(void)
{
    for (int idx = 0; idx < MAX_PHY; ++idx)
    {
        WS2812_Set_Num_LEDS(idx, 0, NULL);
    }
    memset(LEDS_Phy, 0, sizeof(LEDS_Phy));

    LEDS_Buff_Reset();
}


PUBLIC void LED_Init(void)
//
// Call once at startup to setup leds.
{
	WS2812_Init();		// Starts LED driver for all PHYs.
	LEDS_All_Black();   // Start with everything off.
}


// EndFile: Led.c
