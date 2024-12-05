#ifndef WS2812_H
#define WS2812_H

#define WS2812_PIN 14       // First pin for first PHY.
#define MAX_LEDS 600        // Maximum number of LEDS on a string.
#define MAX_PHY 8           // Maximum number of strings.


#define ALL_PHYS (-1)       //
#define CURRENT_PHY 0

extern volatile int Current_PhyNum;   // Current phynum (or logical).  0 for none.

extern void WS2812_Set_Num_LEDS(uint32_t phynum, size_t num_leds);
extern void WS2812_Send(uint32_t phynum, uint32_t* buff);
extern void WS2812_Init(void);

#endif // WS2812_H
