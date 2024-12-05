// LEDS.h

#ifndef LEDS_H
#define LEDS_H

#define LED_VAL uint8_t
#define MAX_LED_VAL 255

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


#define MAX_NUM_LEDS 600

#define LED_SIZE sizeof(LED)          // Size of one LED.
#define LED_DATA_SIZE (Num_LEDS * LED_SIZE)
#define MAX_LED_DATA_SIZE (MAX_NUM_LEDS * LED_SIZE)

// extern size_t Num_LEDS;     // Number of leds used.
// extern LED* LED_Data;       // Current values for LEDs.

extern FLOAT LED_Brightness;       // Brighness level (0-1.0)

extern void PHY_Set_led_count(int phynum, size_t led_count);

extern void LEDS_Set_Phynum(int phynum);

extern size_t Num_LEDS(void);

// Returns an alternate LED_Buffer.
//
//extern LED* ALT_LED_Data(void);    

// Sets alternate buffer to current.
//
//extern void Switch_ALT_LED_Data(void);

// Prepare LEDS for use.
//
extern void LED_Init(void);

// Sets a the number of leds that are used.
//
extern size_t LED_Set_Num_Leds(size_t /*num_leds*/);

// Sets a specific LED to a certain color, led_idx starts at 0
//
extern void LED_Set_RGB(size_t /*led_idx*/, LED_VAL /*r*/, LED_VAL /*g*/, LED_VAL /*b*/);

extern void LED_Set_LED(size_t /*led_idx*/, LED* /*buffp*/);

// Sets all the LEDs to the same color.
//
extern void LED_All_LED(int phynum, LED led);

// Sets all the LEDs to a certain color
//
extern void LED_All_RGB(int phynum, LED_VAL /*r*/, LED_VAL /*g*/, LED_VAL /*b*/);

// Immediatly set all leds to black (off).
//
extern void LEDS_All_Black(void);

// Send scaled LED_Data to led string.
//
extern void LEDS_Do_Update(void);

// Set do_update flag.
//
extern void LED_Update(int phynum);

#endif // LEDS_H
