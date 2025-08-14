// File shifter.c

#include "common.h"
#include "shifter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "led.h"


PRIVATE void do_shift_phys(LED* base, size_t max_leds, LED* padding, int shift_count)
{
    if (shift_count && abs(shift_count) <= max_leds)
    {
        LED* endp = base + max_leds - 1;
       
        if (shift_count > 0)
        {
            LED* dstp = endp;
            LED* srcp = dstp - shift_count;

            while (srcp >= base) { *dstp-- = *srcp--;  }
            if (padding) { while (dstp >= base) { *dstp-- = *padding; } }
        }
        else
        {
            LED* dstp = base;
            LED* srcp = dstp - shift_count;       // shift_count is negative here.

            while (srcp <= endp) { *dstp++ = *srcp++;  }
            if (padding) { while (dstp <= endp) { *dstp++ = *padding; } }
        }
    }
}

extern void mem_dump_ints(void* ptr, size_t n);

#define MAX_SHIFT 100

#define TEMP_SHIFTER_PHY_IDX 0
#define TEMP_SHIFTER_PHY_MASK (1 << TEMP_SHIFTER_PHY_IDX)

PRIVATE void do_Command_Shift_LEDS(bool do_shift, bool do_pad, int shift_count)
//
// Shift (or rotate) led array.
{
    if (do_shift)
    {
        LED padding = {0};
        LED* padp = do_pad ? &padding : NULL;
        size_t max_leds = 0;
        LED* base = LED_Get_Phy(TEMP_SHIFTER_PHY_IDX, &max_leds);


        if (base && max_leds)
        {
            // printf("Before: \n"); mem_dump_ints(base, max_leds);
            do_shift_phys(base, max_leds, padp, shift_count);
            // printf("\nAfter: \n"); mem_dump_ints(base, max_leds); printf("\n");

            LED_Needs_Update(TEMP_SHIFTER_PHY_MASK);
        }
    }
    else
    {
        LED buff[MAX_SHIFT] = {0};
        int abs_shift = abs(shift_count);
        size_t cpsize = abs_shift * sizeof(LED);

        if (abs_shift < MAX_SHIFT)
        {
            size_t max_leds = 0;
            LED* base = LED_Get_Phy(TEMP_SHIFTER_PHY_IDX, &max_leds);

            // printf("Before: \n"); mem_dump_ints(base, max_leds);
            if (shift_count > 0)
            {
                memcpy(buff, base + max_leds - abs_shift, cpsize);     // Save 'tail'.
                do_Command_Shift_LEDS(true, false, shift_count);
                memcpy(base, buff, cpsize);                                // Add saved 'tail' to front.
            }
            else
            {
                memcpy(buff, base, cpsize);                                 // Save 'head'.
                do_Command_Shift_LEDS(true, false, shift_count);
                memcpy(base + max_leds - abs_shift, buff, cpsize);         // Add saved 'head' to end.
            }
            // printf("\nAfter: \n"); mem_dump_ints(base, max_leds); printf("\n");
        }
    }
}


PUBLIC void Command_Shift_LEDS(bool do_shift, int shift_count)
{
    do_Command_Shift_LEDS(do_shift, true, shift_count);
}


// //#define NEXT_P(startp, p) ((++(p) >= ((startp) + Num_LEDS)) ? ((p)=(startp)) : (p))
// #define NEXT_P(startp, p) 
// //	if (count && (abs(count) < Num_LEDS))
// 	{
// //		LED* srcp_start = LED_Data;
// 		LED* srcp_start = 0;
// 		LED* dstp_start = 0; //ALT_LED_Data();

// 		LED* dstp = dstp_start;

// 		if (count > 0)
// 		{
// 			if (do_shift)
// 			{
// 				LED* srcp = srcp_start;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				int c = count;
// 				while (c--)
// 				{
// 					dstp->val = 0;
// 					NEXT_P(dstp_start, dstp);
// 					--i;
// 				}

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					NEXT_P(srcp_start, srcp);
// 					NEXT_P(dstp_start, dstp);
// 				}
// 			}
// 			else
// 			{
// //				LED* srcp = srcp_start + Num_LEDS - count;
// 				LED* srcp = 0;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					NEXT_P(srcp_start, srcp);
// 					NEXT_P(dstp_start, dstp);
// 				}
// 			}
// 		}
// 		else
// 		{
// 			count = -count;

// 			if (do_shift)
// 			{
// 				LED* srcp = srcp_start + count;
// //				LED* end_srcp = srcp_start + Num_LEDS - count;
// 				LED* end_srcp = 0;

// 				while (srcp < end_srcp)
// 				{
// 					dstp->val = srcp->val;
// 					++srcp; ++dstp;
// 				}

// //				while (dstp < (dstp_start + Num_LEDS))
// 				{
// 					dstp->val = 0;
// 					++dstp;
// 				}
// 			}
// 			else
// 			{
// 				LED* srcp = srcp_start + count;
// //				LED* end_srcp = srcp_start + Num_LEDS - count;
// 				LED* end_srcp = 0;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					++dstp;
// 					NEXT_P(srcp_start, srcp);
// 				}
// 			}
// 		}

// //		Switch_ALT_LED_Data();  // Flip to alternate.
// 	}
//}


// End File: shifter.c
