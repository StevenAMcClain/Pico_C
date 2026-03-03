// LEDS.h

#ifndef LEDS_H
#define LEDS_H

#define MAX_PHY 4            // Maximum number of strings.
#define MAX_NUM_LEDS 20000   // Fairly arbitrary maximum for total number of LEDS supported.

#define ALL_PHYS (-1)       // Mask for all physical strings.
#define CURRENT_PHY 0       // Code for use current string (or no string if current is also 0)

#define PHY_IDX_VALID(idx) ((idx) >= 0 && (idx) < MAX_PHY)

#define LED_VAL uint8_t     // Type for a single LED color value.
#define LED_VAL_MAX 255     // Maximum value that can be stored in a LED_VAL

typedef union
{
    struct
    {
        uint8_t phy_num;
        LED_VAL blue;
        LED_VAL red;
        LED_VAL green;
    } led;
    uint32_t val;
} LED;

#define LED_SIZE sizeof(LED)                        // Size of one LED.
#define LED_DATA_SIZE (Num_LEDS * LED_SIZE)

typedef struct morph_step
{
    int8_t white;
    int8_t blue;
    int8_t red;
    int8_t green;

} MORPH_STEP;

typedef struct morph_count
{
    uint8_t white;
    uint8_t blue;
    uint8_t red;
    uint8_t green;

} MORPH_COUNT;

typedef struct led_morph_single
{
    MORPH_STEP  steps;
    MORPH_COUNT counts;
    MORPH_COUNT periods;

} LED_MORPH_SINGLE;

typedef struct led_morph
{
    size_t num_leds;
    LED* dests;
    LED_MORPH_SINGLE* morphs;

} LED_MORPH;

typedef struct
{
	size_t led_count;				// Number of leds on string.
	LED* led_data;					// Data for LED string.
	LED* scaled_led_data;			// Buffer that is actually sent to LEDS.
    LED_MORPH morph_data;           // Data used while morphing.
    int32_t mirroring;              // Phy index + 1 if mirroring else 0.
    uint32_t mirror_mask;           // Mask of mirrored phys.

} LEDS_PHY;

//extern FLOAT LED_Brightness;       // Brighness level (0-1.0)
extern uint32_t LED_Brightness;       // Brighness level (0-1024 == 0-100%)

//extern volatile uint32_t Current_Phy_Mask;  // Current phynum (or logical).  0 for none.

//extern void LEDS_Buff_Reset();
extern LED* LEDS_Buff_Allocate(size_t /*size*/);
extern size_t LEDS_Buff_Available(void);

extern void PHY_Set_Led_Count(int /*val*/);
//
// Set the number of leds on a string.  Allocates LED buffers.

extern LEDS_PHY* LED_Get_Phy(int /*phy_idx*/);
extern size_t PHY_Get_LED_Count(int /*phy_idx*/);

extern LED* LED_Get_LED_Data(int /*phy_idx*/, size_t* /*num_ledsp*/);

extern void LEDS_Do_Update(void);
//
// Sends the data to the LEDs for all PHY that are flagged for update.

extern void LED_Needs_Update(int /*phy_mask*/);
//
// Sets the update flag(s) for phynum.

extern size_t Num_LEDS_Mask(int /*phy_mask*/);

extern void LED_Set_LED_Mask(int /*phy_mask*/, size_t /*led_idx*/, LED* /*source_ledp*/);

extern void LED_Set_RGB_Mask(int /*phy_mask*/, size_t /*led_idx*/, LED_VAL /*r*/, LED_VAL /*g*/, LED_VAL /*b*/);

extern void LED_All_LED_Mask(int /*phy_mask*/, LED /*led*/);
//
// Sets all LEDs to the same color.

extern void LED_All_RGB_Mask(int /*phy_mask*/, LED_VAL /*r*/, LED_VAL /*g*/, LED_VAL /*b*/);
//
// Sets all the LEDs to a certain color.

extern void LEDS_All_Black(void);
//
// Immediately set ALL leds to black (off).

extern void LEDS_Phy_Reset(void);
//
// Clear all phy leds.

extern void PHY_Build_Mirror_Masks(void);
//
// Build mirror masks for all phys.

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


