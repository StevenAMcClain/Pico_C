// File: Scene.c

#include "common.h"
#include "scene.h"

#include <stdio.h>

#include "blob.h"
#include "debug.h"
#include "led.h"


PRIVATE inline void Unpack_Scene_Word(uint32_t val, uint8_t* buff)
//
// Extract scene bytes from uint32 value.
{
	buff[0] = val & 0xFF;
	buff[1] = (val >> 8) & 0xFF;
	buff[2] = (val >> 16) & 0xFF;
	buff[3] = (val >> 24) & 0xFF;
}


PRIVATE void LED_Set_Buff(LED* ledp, uint8_t* buff)
//
// Sets a LED with red, green, and blue.
{
    if (ledp)
    {
		ledp->led.red   = buff[1];	// Red.
		ledp->led.green = buff[2];	// Green.
		ledp->led.blue  = buff[3];	// Blue.

        int phy_mask =  1 << (ledp->led.phy_num - 1);
        LED_Needs_Update(phy_mask);
	}
}


PRIVATE void LEDP_Set_Buff(LED* ledpp, uint8_t* buff)
//
// Sets a LED with red, green, and blue.
{
    LED_Set_Buff(*(LED**)ledpp, buff);
}


PRIVATE void Render_Scene_Ptr(LED* leds, size_t num_leds, uint32_t* scene_start_ptr)
//
// Render scene given pointer to start of scene script.
{
    LED* ledp = leds;
	uint32_t* scenep = scene_start_ptr;

	int leds_left = num_leds;

	while (leds_left > 0)
	{
		uint8_t buff[4];

		Unpack_Scene_Word(*scenep++, buff);

		uint8_t flags = buff[0];

		if (flags == FLAGS_END)
        {
            break; 
        }
		else if (flags == FLAGS_END_ALL)  
		{
            if ((scenep - 1) == scene_start_ptr) { break; } // Just end all
			scenep = scene_start_ptr;
		}
		else if (flags == FLAGS_END_LAST)
		{
			while (leds_left-- > 0) { LED_Set_Buff(ledp++, buff); }
		}
		else if (flags == FLAGS_SKIP)
		{
            ledp += buff[1];
            leds_left -= (buff[1] < leds_left) ? buff[1] : leds_left;
		}
		else
		{
			int count = flags ? flags : 1;
            while (count-- && leds_left-- > 0) { LED_Set_Buff(ledp++, buff); }
		}
	}
}


PRIVATE LEDS_PHY* Render_Scene_Idx(uint32_t idx, uint32_t* scene_start_ptr)
{
    LEDS_PHY* phy = LED_Get_Phy(idx);

    if (phy) { Render_Scene_Ptr(phy->led_data, phy->led_count, scene_start_ptr); }

    return phy;
}


PRIVATE void Render_Scene_Mask(uint32_t phy_mask, uint32_t* scene_start_ptr)
//
// Render scene given pointer to start of scene script.
{
    uint32_t mask = 1;
	int phy_idx = 0;

	while (phy_idx < MAX_PHY && phy_mask)
	{
        if (mask & phy_mask)
        {
            LEDS_PHY* phy = Render_Scene_Idx(phy_idx, scene_start_ptr);

            if (phy) { phy_mask &= phy->mirror_mask; }
            else     { phy_mask &= ~mask;            }
        }
        ++phy_idx;  mask <<= 1;
    }
}



PUBLIC void Set_Scene_mask(uint32_t phy_mask, SCENE_ID scene_id)
//
// .
{
	if (scene_id && scene_id <= Blob.Num_Scenes)
	{
        int scene_idx = (--scene_id) * 2;
		uint32_t start_idx = Blob.Scene_Index[scene_idx + 1];

        if (start_idx) { Render_Scene_Mask(phy_mask, Blob.Scene_Array + start_idx - 1); }
    }
}


// EndFile: Scene.c







#ifdef COMMENT
PRIVATE void Render_Scene(int phy_mask, uint32_t* start_ptr)
//
// Render scene given pointer to start of scene script.
{
	uint32_t* ptr = start_ptr;

	uint32_t i = 0;

    size_t num_leds = Num_LEDS_Mask(phy_mask);

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
				LED_Set_RGB_Mask(phy_mask, i, buff[1], buff[2], buff[3]);
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
				LED_Set_RGB_Mask(phy_mask, i, buff[1], buff[2], buff[3]);
				++i;
//				D(PRINTF(".");)
			}
//			D(PRINTF("\n");)
		}
	}
}


PUBLIC void Render_Scene_ptr(LED* leds, size_t num_leds, SCENE_ID id)
{
	if (id && id <= Blob.Num_Scenes)
	{
        int idx = (--id) * 2;
		uint32_t start_idx = Blob.Scene_Index[idx + 1];

        D(DEBUG_SCENES, PRINTF("Render_Scene_ptr: id = %d, start_idx= %d\n", id, start_idx);)

        if (start_idx) 
        {
            uint32_t* scene_start_ptr = Blob.Scene_Array + start_idx - 1;
            Render_Scene_Ptr(leds, num_leds, scene_start_ptr);
        }
	}
	else { D(DEBUG_SCENES, PRINTF("Render_Scene_ptr: bad id = %d\n", id);) }
}
#endif // COMMENT


// PUBLIC void Set_Scene_idx(int phy_idx, SCENE_ID id)
// //
// // Render scene given scene id.
// {
// 	if (id && id <= Blob.Num_Scenes)
// 	{
//         int idx = (--id) * 2;
// 		uint32_t start_idx = Blob.Scene_Index[idx + 1];

//         D(DEBUG_SCENES, PRINTF("Set scene: id = %d, start_idx= %d\n", id, start_idx);)

//         if (start_idx) { Render_Scene_Idx(phy_idx, Blob.Scene_Array + start_idx - 1); }
// 	}
// 	else { D(DEBUG_SCENES, PRINTF("Set scene: bad id = %d\n", id);) }
// }
// 	int phy_idx = 0;
//     uint32_t mask = 1;
//         while (phy_mask && phy_idx++ < MAX_PHY)
// 	{
//         if (mask & phy_mask)
//         {
//             Set_Scene_idx(phy_idx, scene_id);
//             LEDS_PHY* phy = LED_Get_Phy(phy_idx);
//             if (phy) { phy_mask &= phy->mirror_mask; }
//             else     { phy_mask &= ~mask;            }
//         }
//         mask <<= 1;
//     }
// }

