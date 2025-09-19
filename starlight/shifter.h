// File: shifter.h

#ifndef SHIFTER_H
#define SHIFTER_H

#define MAX_LED_ROTATE 100

#include "led.h"

extern void Command_Shift_LEDS(int phy_idx, bool do_shift, int shift_count, LED* buff);
extern void Command_Shift_LEDS_mask(int phy_mask, bool do_shift, int shift_count, LED* buff);

#endif // SHIFTER_H

// End File: shifter.h