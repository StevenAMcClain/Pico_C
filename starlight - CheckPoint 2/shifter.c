// File shifter.c

#include "common.h"
#include "shifter.h"

#include <stdio.h>
#include <stdlib.h>


extern void mem_dump_ints(void* ptr, size_t n);


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
            while (dstp >= base) { *dstp-- = *padding; }
        }
        else
        {
            LED* dstp = base;
            LED* srcp = dstp - shift_count;       // shift_count is negative here.

            while (srcp <= endp) { *dstp++ = *srcp++;  }
            while (dstp <= endp) { *dstp++ = *padding; }
        }
    }
}


PRIVATE void do_rotate_phys(LED* base, size_t max_leds, LED* buff, int shift_count)
{
    LED* bptr = buff;

    if (shift_count && abs(shift_count) <= MAX_LED_ROTATE)
    {
        LED* endp = base + max_leds - 1;
       
        if (shift_count > 0)
        {
            LED* dstp = endp;
            LED* srcp = dstp - shift_count;

            while (srcp <= endp) { *bptr++ = *srcp++;  }

            srcp = dstp - shift_count;
            while (srcp >= base) { *dstp-- = *srcp--;  }

            bptr = buff;
            while (dstp >= base) { *dstp-- = *bptr++; }
        }
        else
        {
            LED* srcp = base;       // shift_count is negative here.
            LED* stopp = base - shift_count - 1;

            while (srcp <= stopp) { *bptr++ = *srcp++;  }

            LED* dstp = base;

            srcp = dstp - shift_count;       // shift_count is negative here.
            while (srcp <= endp) { *dstp++ = *srcp++;  }

            bptr = buff;
            while (dstp <= endp) { *dstp++ = *bptr++; }
        }
    }
}


PUBLIC void Command_Shift_LEDS(int phy_idx, bool do_shift, int shift_count, LED* buff)
//
// Shift (or rotate) led array on one phy.
{
    LED padding = {0};
    size_t max_leds = 0;
    LED* base = LED_Get_LED_Data(phy_idx, &max_leds);

    if (base && max_leds)
    {
//        PRINTF("Before: \n");   mem_dump_ints(base, max_leds);

        if (do_shift) { do_shift_phys(base, max_leds, &padding, shift_count); }
        else          { do_rotate_phys(base, max_leds, buff, shift_count);    }

//        PRINTF("\nAfter: \n");  mem_dump_ints(base, max_leds);  PRINTF("\n");

        LED_Needs_Update(1 << phy_idx);
    }
}


PUBLIC void Command_Shift_LEDS_mask(int phy_mask, bool do_shift, int shift_count, LED* buff)
//
// Shift (or rotate) led array on many phys (one bit in mask per).
{
	int phy_idx = 0;
    uint32_t mask = 1;

	while (phy_mask && phy_idx < MAX_PHY)
	{
        if (mask & phy_mask)
        {
            Command_Shift_LEDS(phy_idx, do_shift, shift_count, buff);
            phy_mask &= ~mask;
        }
        mask <<= 1;     ++phy_idx;
    }
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
