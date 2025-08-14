#ifndef WS2812_H
#define WS2812_H

#define WS2812_PIN 14       // First pin for first PHY.

extern void WS2812_Set_Num_LEDS(int phy_idx, size_t num_leds);
extern void WS2812_Prime_Send(uint32_t phy_mask, uint32_t* buff);
extern void WS2812_Do_Send(void);
extern void WS2812_Init(void);

#endif // WS2812_H
