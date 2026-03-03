/**
 * Started as example code and was modified by Steven A. McClain
 *  - definitely not backward compatible with anything.
 * Dec 2024
 */

#include "Common.h"
#include "ws2812.h"

#include <stdio.h>

#include <hardware/dma.h>
#include <hardware/pio.h>

#include <pico/sem.h>

#include "ws2812.pio.h"
#include "led.h"

#define WS2812_TICK_TIME 50  // milliseconds.

#define IS_RGBW false

PRIVATE volatile uint32_t Primed_Mask = 0;


typedef struct
{
    size_t num_leds;        // Number of leds on this PHY.

    PIO pio;                // PIO unit for this PHY.
    int state_machine;      // State machine number for PHY.

    int DMA_channel;        // DMA channel for PHY.

    uint32_t* buff;         // Buffer to send.

} WS2812_PHY;


PRIVATE WS2812_PHY WS2812_Phy[MAX_PHY] = {0};


PRIVATE void DMA_Init(WS2812_PHY* phy)
{
    // Get a free channel, panic() if there are none
    int chan = phy->DMA_channel = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(chan);
    PIO pio = phy->pio;

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, phy->state_machine, true));
   
    dma_channel_configure(
        chan,                           // Channel to be configured
        &cfg,                           // The configuration we just created
        &pio->txf[phy->state_machine],  // The initial write address
        0,                              // The initial read address
        0,                              // Number of transfers; in this case each is 4 bytes.
        false                           // Don't start immediately.
    );

 //   dma_channel_set_irq0_enabled(chan, true);
}


PUBLIC void WS2812_Set_Num_LEDS(int phy_idx, size_t num_leds, uint32_t* buff)
{
    if (phy_idx >= 0 && phy_idx < MAX_PHY)
    {
        WS2812_PHY* phy = WS2812_Phy + phy_idx;

        phy->num_leds = num_leds;
        phy->buff = buff;
    }
}


PUBLIC void WS2812_Set_Primed(uint32_t phy_mask)
{
    Primed_Mask |= phy_mask;
}


PRIVATE bool WS2812_Tick(struct repeating_timer* ptr)
{
    if (Primed_Mask)
    {
        WS2812_PHY* phy = WS2812_Phy;
        int phy_idx = 0;
        int mask = 1;
        uint32_t start_mask = 0;

        while (phy_idx < MAX_PHY)
        {
            if ((mask & Primed_Mask) && phy->num_leds)
            {
                uint chan = phy->DMA_channel;
                int num_leds = phy->num_leds;
                uint32_t* buff = phy->buff;

                dma_channel_set_trans_count(chan, num_leds, false);
                dma_channel_set_read_addr(chan, buff, false);
                start_mask |= (1 << chan);
            }
            ++phy; ++phy_idx; mask <<= 1;
        }
        dma_start_channel_mask(start_mask);                // Trigger DMA transfer(s).
        Primed_Mask = 0;
    }
    return true;
}


// PUBLIC void WS2812_TestSend(int chan, size_t num_leds, uint32_t* buff)
// {
//     dma_channel_set_trans_count(chan, num_leds, false);
//     dma_channel_set_read_addr(chan, buff, true);                // Trigger DMA transfer.
// }


PRIVATE void WS2812_Tick_Init()
{
	static struct repeating_timer timer_1;		// Timer to call Blob_Time_Tick.
	add_repeating_timer_us(WS2812_TICK_TIME, WS2812_Tick, NULL, &timer_1);
}

// PUBLIC void PIO_Send_One(uint32_t value)
// {
//     pio_sm_put_blocking(pio0, 0, value);
// }

PUBLIC void WS2812_Init(void)
{
    uint pins[MAX_PHY] = {16, 17, 18, 19};
    uint offset = pio_add_program(pio0, &ws2812_program);
    offset = pio_add_program(pio1, &ws2812_program);

    WS2812_PHY* phy = WS2812_Phy;
    int i = 0;

    while (i < MAX_PHY)
    {
        PIO pio = phy->pio = (i < 4) ? pio0 : pio1;
        int sm = phy->state_machine = i % 4;

        bool is_enabled = (pio->ctrl & (1u << sm)) != 0;
        bool is_claimed = pio_sm_is_claimed(pio, sm);

        if (is_enabled) { printf("\n\n *** WS2812_Init: already enabled pio %d, sm %d\n\n\n", pio, sm); }
        else if (is_claimed) { printf("\n\n *** WS2812_Init: already claimed pio %d, sm %d\n\n\n", pio, sm); }
        else
        {
            pio_sm_claim(pio, sm);
            ws2812_program_init(pio, sm, offset, pins[i], 800000, IS_RGBW);
            DMA_Init(phy);
        }
        ++phy; ++i;
    }

    WS2812_Tick_Init();

    sleep_ms(10);		// Wait a bit to ensure clock is running and force LEDs to reset
}


// EndFile: ws2812.c
