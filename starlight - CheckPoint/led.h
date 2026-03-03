// LEDS.h

#ifndef LEDS_H
#define LEDS_H

#include "common.h"

#define MAX_PHY 8           // Maximum number of strings.
#define MAX_NUM_LEDS 1000  // Fairly arbitrary maximum for one physical string of LEDS.

#define ALL_PHYS (-1)       // Mask for all physical strings.
#define CURRENT_PHY 0       // Code for use current string (or no string if current is also 0)

//extern volatile uint32_t Current_Phy_Mask;   // Current phynum (or logical).  0 for none.

#define LED_VAL uint8_t     // Type for a single LED color value.
#define LED_VAL_MAX 255     // Maximum value that can be stored in a LED_VAL

typedef union
{
    struct
    {
        LED_VAL white;
        LED_VAL blue;
        LED_VAL red;
        LED_VAL green;
    } led;
    uint32_t val;
} LED;


#define LED_SIZE sizeof(LED)                        // Size of one LED.
#define LED_DATA_SIZE (Num_LEDS * LED_SIZE)
#define MAX_LED_DATA_SIZE (MAX_NUM_LEDS * LED_SIZE)

extern FLOAT LED_Brightness;       // Brighness level (0-1.0)

//extern volatile uint32_t Current_Phy_Mask;  // Current phynum (or logical).  0 for none.

extern size_t PHY_Get_LED_Count(int phy_idx);

extern void LEDS_Buff_Reset();
extern LED* LEDS_Buff_Allocate(size_t size);

extern void PHY_Set_led_count(int phy_idx, size_t led_count);
//
// Set the number of leds on a string.  Allocates LED buffers.

//extern void LEDS_Set_Phynum(int phy_mask);

extern LED* LED_Get_LED_Data(int phy_idx, size_t* num_ledsp);

extern void LEDS_Do_Update(void);
//
// Sends the data to the LEDs for all PHY that are flagged for update.

extern void LED_Needs_Update(int phy_mask);
//
// Sets the update flag(s) for phynum.

extern size_t Num_LEDS_Mask(int phy_mask);

extern void LED_Set_LED(size_t led_idx, LED* source_ledp);

extern void LED_Set_RGB(size_t led_idx, LED_VAL r, LED_VAL g, LED_VAL b);

extern void LED_All_LED_Mask(int phy_mask, LED led);
//
// Sets all LEDs to the same color.

extern void LED_All_RGB_Mask(int phynum, LED_VAL r, LED_VAL g, LED_VAL b);
//
// Sets all the LEDs to a certain color.

extern void LEDS_All_Black();
//
// Immediately set ALL leds to black (off).

extern void LED_Init(void);
//
// Call once at startup to setup leds.

#endif // LEDS_H


// Returns an alternate LED_Buffer.
//
//extern LED* ALT_LED_Data(void);    

// Sets alternate buffer to current.
//
//extern void Switch_ALT_LED_Data(void);


