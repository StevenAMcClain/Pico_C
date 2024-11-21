#ifndef WS2812_H
#define WS2812_H

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
// default to pin 14 if the board doesn't have a default WS2812 pin defined
#define WS2812_PIN 14
#endif

extern void sw2812_Set_Num_LEDS(size_t num_leds);
extern void sw2812_Send(uint32_t* buff);
extern void sw2812_Init(size_t num_leds);

#endif // WS2812_H
