// File: Scene.c

#include "common.h"
#include "scene.h"

#include <stdio.h>

#include "blob.h"
#include "debug.h"
#include "led.h"


PRIVATE void Unpack_Scene(uint32_t val, uint8_t* buff)
//
// Extract scene bytes from uint32 value.
{
	buff[0] = val & 0xFF;
	buff[1] = (val >> 8) & 0xFF;
	buff[2] = (val >> 16) & 0xFF;
	buff[3] = (val >> 24) & 0xFF;
}


PRIVATE void Render_Scene(int phy_mask, uint32_t* start_ptr)
//
// Render scene given pointer.
{
	uint32_t* ptr = start_ptr;

	uint32_t i = 0;

    size_t num_leds = Num_LEDS(phy_mask);

	while (i < num_leds)
	{
		uint8_t buff[4];

		Unpack_Scene(*ptr++, buff);

		uint8_t flags = buff[0];

		if (flags == FLAGS_END)
		{
//			D(PRINTF("End\n");)
			break;
		}
		else if (flags == FLAGS_END_ALL)  
		{
//			D(PRINTF("End All\n");)
            if ((ptr - 1) == start_ptr) break;  // Just end all
			ptr = start_ptr;
		}
		else if (flags == FLAGS_END_LAST)
		{
//			D(PRINTF("End Last");)
			while (i < num_leds)
			{
				LED_Set_RGB(phy_mask, i, buff[1], buff[2], buff[3]);
				++i;
//				D(PRINTF(".");)
			}
//			D(PRINTF("\n");)
		}
		else if (flags == FLAGS_SKIP)
		{
//			D(PRINTF("skip %d\n", buff[1]);)
				i += buff[1];
		}
		else
		{
//    		D(PRINTF(": i = %d, ", i);)
//			D(PRINTF("%d %d %d", buff[1], buff[2], buff[3]);)
			int count = flags ? flags : 1;
			while (count--)
			{
				LED_Set_RGB(phy_mask, i, buff[1], buff[2], buff[3]);
				++i;
//				D(PRINTF(".");)
			}
//			D(PRINTF("\n");)
		}
	}
}


PUBLIC void Set_Scene(int phy_idx, SCENE_ID id)
//
// Render scene given scene id.
{
	if (id && id <= Blob.Num_Scenes)
	{
        int idx = (--id) * 2;
		uint32_t start_idx = Blob.Scene_Index[idx + 1];

        D(DEBUG_SCENES, PRINTF("Set scene: id = %d, start_idx= %d\n", id, start_idx);)

        if (start_idx)
        {
    		Render_Scene(phy_idx, Blob.Scene_Array + start_idx - 1);
        }
	}
	else { D(DEBUG_SCENES, PRINTF("Set scene: bad id = %d\n", id);) }
}


PUBLIC void Set_Scene_mask(int phy_mask, SCENE_ID scene_id)
//
// Shift (or rotate) led array on one phy.
{
	int phy_idx = 0;
    uint32_t mask = 1;

	while (phy_mask && phy_idx++ < MAX_PHY)
	{
        if (mask & phy_mask)
        {
            Set_Scene(phy_idx, scene_id);
            phy_mask &= ~mask;
        }
        mask <<= 1;
    }

}


// EndFile: Scene.c
