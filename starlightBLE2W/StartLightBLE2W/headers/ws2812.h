#ifndef WS2812_H
#define WS2812_H

#include "picopio.h"

#define MAX_LED_STRIPS 4

typedef struct ws2812_strip
{
    pio_handle_t* pio;    // pio_handle.

    int pin;             // Pin number for led strip.
    size_t count;        // Number of elements (uint32_t)
    uint32_t* buff;      // Buffer to send.

} ws2812_strip_t;

extern bool WS2812_Setup_Pio(uint pin, uint num_leds, bool is_rgbw);
extern void WS2812_Set_Dirty_Mask(uint32_t phy_mask);
extern void WS2812_Set_Dirty_Pin(uint8_t pin);
extern ws2812_strip_t* WS2812_Get_Led_Strip(uint8_t pin);


// extern void WS2812_Init(void);
// extern void WS2812_Set_Num_LEDS(int phy_idx, size_t num_leds, uint32_t* buff);
// extern void WS2812_Prime_Send(uint32_t phy_mask, uint32_t* buff);
// extern void WS2812_Do_Send(void);
// extern void WS2812_Set_Primed(uint32_t phy_mask);

#endif // WS2812_H
