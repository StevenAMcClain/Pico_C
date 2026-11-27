// File: ws2812.c

#include "common.h"
#include "ws2812.h"

#include <stdlib.h>
#include <string.h>

#include <hardware/dma.h>
#include <pico/critical_section.h>

#include "ws2812.pio.h"

#include "balloc.h"
#include "error.h"


#define WS2812_TICK_TIME 10  // milliseconds.


PRIVATE ws2812_strip_t Led_Strip[MAX_LED_STRIPS] = {0};
PRIVATE ws2812_strip_t* Next_Led_Strip = Led_Strip;
PRIVATE size_t Led_Strip_Count = 0;

PRIVATE volatile uint32_t Dirty_Strips = 0;
PRIVATE critical_section_t Dirty_Strips_Crit_Sec;


PRIVATE ws2812_strip_t* get_next_led_strip(void)
{
    if (Led_Strip_Count < MAX_LED_STRIPS)
    {
        ++Led_Strip_Count;
        return Next_Led_Strip++;
    }
    return NULL;
}


PUBLIC ws2812_strip_t* WS2812_Get_Led_Strip(uint8_t pin)
{
    ws2812_strip_t* ptr = Led_Strip;
    int count = Led_Strip_Count;

    while (count--)
    {
        if (ptr->pin == pin) { return ptr; }

        ++ptr;
    }
    return NULL;
}

int DMA_Busy_Event = 0;


PRIVATE bool WS2812_Tick(struct repeating_timer* rt)
{
    UNUSED(rt);

    uint32_t dirty;

    // Snapshot-and-clear the dirty mask under lock, then work from the local
    // copy � keeps the critical section short and avoids re-reading a mask
    // that a concurrent Set_Dirty call could still be modifying.
    critical_section_enter_blocking(&Dirty_Strips_Crit_Sec);
    dirty = Dirty_Strips;
    Dirty_Strips = 0;
    critical_section_exit(&Dirty_Strips_Crit_Sec);

    if (dirty)
    {
        ws2812_strip_t* strip = Led_Strip;
        int idx = 0;
        uint32_t mask = 1;

        while (idx < MAX_LED_STRIPS)
        {
            if ((mask & dirty) && strip->count)
            {
                if (dma_channel_is_busy(strip->pio->DMA_Write)) 
                { 
                    ++DMA_Busy_Event; 
                    return true; 
                }

                PIO_DMA_Writer_Prime(strip->pio->DMA_Write, strip->buff, strip->count);
            }
            ++strip; ++idx; mask <<= 1;
        }
        
        PIO_DMA_Writer_Start();
    }
    return true;
}


PRIVATE void WS2812_Tick_Init()
{
    static bool is_running = false;
	static struct repeating_timer timer_1;		// Timer to call Blob_Time_Tick.

    if (!is_running)
    {
        critical_section_init(&Dirty_Strips_Crit_Sec);
        add_repeating_timer_ms(WS2812_TICK_TIME, WS2812_Tick, NULL, &timer_1);
        is_running = true;
    }
}


PUBLIC void WS2812_Set_Dirty_Mask(uint32_t mask)
{
    critical_section_enter_blocking(&Dirty_Strips_Crit_Sec);
    Dirty_Strips |= mask;
    critical_section_exit(&Dirty_Strips_Crit_Sec);
}


PUBLIC void WS2812_Set_Dirty_Pin(uint8_t pin)
{
    ws2812_strip_t* ptr = Led_Strip;
    int count = Led_Strip_Count;
    int idx = 0;

    while (count--)
    {
        if (ptr->pin == pin) 
        {
            critical_section_enter_blocking(&Dirty_Strips_Crit_Sec);
            Dirty_Strips |= 1 << idx;
            critical_section_exit(&Dirty_Strips_Crit_Sec);
            return;
        }

        ++ptr; ++idx;
    }
    POST_ERROR("Can't find pin %d\n", pin);
}


PUBLIC bool WS2812_Setup_Pio(uint pin, uint num_leds, bool is_rgbw)
{
    bool result = false;
    ws2812_strip_t* strip = get_next_led_strip();

    if (strip)
    {
        strip->pin = pin;

        uint32_t len = num_leds * sizeof(uint32_t);

        strip->buff = Ballocz(PSRAM_Balloc, len);

        if (strip->buff)
        {
            memset(strip->buff, 0, len);

            strip->count = num_leds;

            pio_handle_t* pio = PIO_Allocate(&ws2812_program);

            if (pio)
            {
                strip->pio = pio;

                PIO pio_device = pio->pio_device;
                pio_state_machine_index state_machine = pio->state_machine;
                uint offset = pio->offset;

                ws2812_program_init(pio_device, state_machine, offset, pin, 800000, is_rgbw);
                PIO_DMA_Writer_Init(strip->pio);

                WS2812_Tick_Init();
                result = true;
            }
            else { POST_ERROR("Can't allocate a PIO!"); }
        }
        else { POST_ERROR("Can't allocate a buffer!"); }
    }
    else { POST_ERROR("Can't allocate a led_strip!"); }

    return result;
}


// EndFile: ws2812.c


//======================================================================================================

// #ifdef PICO_RP2350
// #define MAX_PIO 12
// #define MAX_PIO_DEVICES 3
// #else
// #define MAX_PIO_CHANNELS 8
// #define MAX_PIO_DEVICES 2
// #endif


//#define IS_RGBW false

//#define MAX_PIO_PROGRAM_INSTRUCTIONS 32

// typedef struct pio_memory
// {
//     uint16_t Program_Bytes[MAX_PIO_PROGRAM_INSTRUCTIONS];

// } pio_memory_t;

// PRIVATE pio_memory_t Pio_Memory[MAX_PIO_DEVICES];



// // Returns the offset where `needle` first appears inside `haystack`,
// // or -1 if not found.
// PRIVATE int buffer_find_offset(const uint16_t *haystack, size_t hay_len,
//                                const uint16_t *needle,   size_t nee_len)
// {
//     if (nee_len == 0) return 0;          // empty needle matches at start
//     if (nee_len > hay_len) return -1;    // cannot fit

//     size_t limit = hay_len - nee_len + 1;

//     for (size_t i = 0; i < limit; ++i)
//     {
//         bool match = true;

//         for (size_t j = 0; j < nee_len; ++j)
//         {
//             if (haystack[i + j] != needle[j])
//             {
//                 match = false;
//                 break;
//             }
//         }

//         if (match) { return (int)i; }    // found at offset i
//     }

//     return -1;                           // not found
// }


// PUBLIC uint Load_Pio_Program(uint8_t pio_device, const pio_program_t *program)
// {
//     uint offset = -1;

//     if (pio_device < MAX_PIO_DEVICES)
//     {
//         pio_memory_t* pio_memory = &Pio_Memory[pio_device];
//         const uint16_t* instructions = program->instructions;
//         uint8_t length = program->length;

//         uint offset = buffer_find_offset(pio_memory->Program_Bytes, MAX_PIO_PROGRAM_INSTRUCTIONS, instructions, length);

//         if (offset < 0)
//         {
// #ifdef PICO_RP2350
//             PIO pio = (pio_device == 0) ? pio0 : (pio_device == 1) ? pio1 : pio2;
// #else
//             PIO pio = (pio_device == 0) ? pio0 : pio1;
// #endif
//             offset = pio_add_program(pio, program);

//             if (offset > 0 && offset < (MAX_PIO_PROGRAM_INSTRUCTIONS - length))
//             {
//                 memcpy(pio_memory->Program_Bytes + offset, instructions, length);
//             }
//         }
//     }

//     return offset;
// }

// typedef struct pio_handle
// {
//     PIO pio;                // PIO unit for this PHY.
  
//     int state_machine;      // State machine number for PHY.

//     int DMA_channel;        // DMA channel for PHY.

// } pio_handle_t;


// PRIVATE pio_handle_t  Pio_Handle[MAX_PIO_CHANNELS] = {0};   // Array of pio records.
// PRIVATE pio_handle_t* Next_Pio = Pio_Handle;       // Points to next pio to use.
// PRIVATE size_t        Pio_Count = 0;        // Number of pios allocated. 


// PRIVATE pio_handle_t* Alocate_Next_Pio_Handle(void)
// {
//     pio_handle_t* result = NULL;

//     if (Pio_Count < MAX_PIO_CHANNELS)
//     {
//         ++Pio_Count;
//         result = Next_Pio++;
//     }

//     return result;
// }


// PRIVATE void PIO_DMA_Writer_Init(pio_handle_t* phy)
// {
//     // Get a free channel, panic() if there are none
//     int chan = phy->DMA_channel = dma_claim_unused_channel(true);
//     dma_channel_config cfg = dma_channel_get_default_config(chan);
//     PIO pio = phy->pio;

//     channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
//     channel_config_set_read_increment(&cfg, true);
//     channel_config_set_write_increment(&cfg, false);
//     channel_config_set_dreq(&cfg, pio_get_dreq(pio, phy->state_machine, true));
   
//     dma_channel_configure(
//         chan,                           // Channel to be configured
//         &cfg,                           // The configuration we just created
//         &pio->txf[phy->state_machine],  // The initial write address
//         0,                              // The initial read address
//         0,                              // Number of transfers; in this case each is 4 bytes.
//         false                           // Don't start immediately.
//     );
// }


// PUBLIC bool PIO_Setup_Pio(int index)
// {
//     bool result = false;

//     if (index < MAX_PIO_CHANNELS)
//     {
//         pio_handle_t* pio =  &Pio_Handle[index];
// #ifdef PICO_RP2350
//         PIO pio_device = phy->pio = (index < 4) ? pio0 : (index < 8) ? pio1 : pio2;
// #else
//         PIO pio_device = pio->pio = (index < 4) ? pio0 : pio1;
// #endif
//         int state_machine = pio->state_machine = index % 4;

//         bool is_enabled = (pio_device->ctrl & (1u << state_machine)) != 0;
//         bool is_claimed = pio_sm_is_claimed(pio_device, state_machine);

//              if (is_enabled) { printf("\n\n *** WS2812_Init: already enabled pio %d, sm %d\n\n\n", pio_device, state_machine); }
//         else if (is_claimed) { printf("\n\n *** WS2812_Init: already claimed pio %d, sm %d\n\n\n", pio_device, state_machine); }
//         else
//         {
//             pio_sm_claim(pio_device, state_machine);
// //            ws2812_program_init(pio, sm, program_offset, pin, 800000, is_rgbw);
// //            PIO_DMA_Init(phy);
//             result = true;
//         }
//     }
//     return result;
// }

    // PIO pio;                // PIO unit for this PHY.
    // uint pio_offset;        // Start of code for pio.
    // int state_machine;      // State machine number for PHY.
    // int DMA_channel;        // DMA channel for PHY.

// PRIVATE void DMA_Init(WS2812_PHY* phy)
// {
//     // Get a free channel, panic() if there are none
//     int chan = phy->DMA_channel = dma_claim_unused_channel(true);
//     dma_channel_config cfg = dma_channel_get_default_config(chan);
//     PIO pio = phy->pio;

//     channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
//     channel_config_set_read_increment(&cfg, true);
//     channel_config_set_write_increment(&cfg, false);
//     channel_config_set_dreq(&cfg, pio_get_dreq(pio, phy->state_machine, true));
   
//     dma_channel_configure(
//         chan,                           // Channel to be configured
//         &cfg,                           // The configuration we just created
//         &pio->txf[phy->state_machine],  // The initial write address
//         0,                              // The initial read address
//         0,                              // Number of transfers; in this case each is 4 bytes.
//         false                           // Don't start immediately.
//     );
// }


    // uint offset = pio_add_program(pio0, &ws2812_program);
    //      offset = pio_add_program(pio1, &ws2812_program);

    // int i = 0;
    // uint pins[MAX_PHY] = {16, 17, 18, 19};

    // while (i < MAX_PHY)
    // {
    //     ++phy; ++i;
    // }

                // int num_leds = phy->num_leds;
                // uint32_t* buff = phy->buff;
                // dma_channel_set_trans_count(chan, num_leds, false);
                // dma_channel_set_read_addr(chan, buff, false);


                

// PUBLIC void WS2812_Init(void)
// {
//     uint offset = pio_add_program(pio0, &ws2812_program);
//          offset = pio_add_program(pio1, &ws2812_program);

//     WS2812_PHY* phy = WS2812_Phy;
//     int i = 0;
//     uint pins[MAX_PHY] = {16, 17, 18, 19};

//     while (i < MAX_PHY)
//     {
//         PIO pio = phy->pio_handle.pio;
//         int sm = phy->pio_handle.state_machine;

//         ws2812_program_init(pio, sm, offset, pins[i], 800000, IS_RGBW);
//         PIO_DMA_Writer_Init(&phy->pio_handle);
        
//         ++phy; ++i;
//     }

//     WS2812_Tick_Init();

//     sleep_ms(10);		// Wait a bit to ensure clock is running and force LEDs to reset
// }
// #include <stdio.h>
// #include <string.h>

// #include <hardware/dma.h>
// #include <hardware/pio.h>

// #include <pico/sem.h>

//PRIVATE volatile uint32_t Primed_Mask = 0;
// PUBLIC void WS2812_Set_Num_LEDS(int phy_idx, size_t num_leds, uint32_t* buff)
// {
//     if (phy_idx >= 0 && phy_idx < MAX_PHY)
//     {
//         ws2812_strip_t* phy = WS2812_Phy + phy_idx;

//         phy->num_leds = num_leds;
//         phy->buff = buff;
//     }
// }



