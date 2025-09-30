#ifndef WS2812_H
#define WS2812_H

extern void WS2812_Set_Num_LEDS(int phy_idx, size_t num_leds, uint32_t* buff);
extern void WS2812_Prime_Send(uint32_t phy_mask, uint32_t* buff);
extern void WS2812_Do_Send(void);
extern void WS2812_Init(void);
extern void WS2812_Set_Primed(uint32_t phy_mask);

#endif // WS2812_H
