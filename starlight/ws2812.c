/**
 * Started as example code and was modified by Steven A. McClain
 *  - definitely not backward compatible with anything.
 * Dec 2024
 */

#include "Common.h"
#include "ws2812.h"

#include <stdio.h>

#include <hardware/dma.h>
#include <pico/sem.h>
#include <hardware/pio.h>

#include "ws2812.pio.h"
#include "led.h"

#define IS_RGBW false

PRIVATE volatile absolute_time_t last_dma_completed_time = 0;
#define DMA_SEND_TIME 1000000
#define DMA_DELAY_TIME 400

typedef struct
{
    size_t num_leds;                                    // Number of leds on this PHY.

    PIO pio;                                            // PIO unit for this PHY.
    int state_machine;                                  // State machine number for PHY.

    int DMA_channel;                                    // DMA channel for PHY.

} WS2812_PHY;


PRIVATE WS2812_PHY WS2812_Phy[MAX_PHY] = {0};


PRIVATE void __isr dma_complete_handler() 
{
    int interrupts = dma_hw->ints0;

    last_dma_completed_time = get_absolute_time() + DMA_DELAY_TIME;

    dma_hw->ints0 = interrupts;        // clear IRQ
}


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
        MAX_NUM_LEDS,                   // Number of transfers; in this case each is 4 bytes.
        false                           // Don't start immediately.
    );

    dma_channel_set_irq0_enabled(chan, true);
}


PUBLIC void WS2812_Set_Num_LEDS(int phyidx, size_t num_leds)
{
    if (phyidx >= 0 && phyidx < MAX_PHY)
    {
        WS2812_PHY* phy = WS2812_Phy + phyidx;

        phy->num_leds = num_leds;
        dma_channel_set_trans_count(phy->DMA_channel, num_leds, false);
    }
}

PRIVATE volatile uint32_t Primed = 0;

PUBLIC void WS2812_Prime_Send(uint32_t phynum, uint32_t* buff)
{
    if (phynum)
    {
        while (phynum)
        {
            WS2812_PHY* phy = WS2812_Phy;
            uint32_t mask = 1;
            int i = 0;

            while (phynum && i < MAX_PHY)
            {
                if (phynum & mask)
                {
                    uint chan = phy->DMA_channel;

                    if (phy->num_leds)
                    {
                        if (!dma_channel_is_busy(chan))
                        {
                            phynum &= ~mask;
                            Primed |= mask;
                            dma_channel_set_read_addr(chan, buff, false);
                        }
                    }
                    else { phynum &= ~mask; }  // Ignore zero leds.
                }

                ++i;  mask <<= 1; ++phy;
            }
            if (phynum)
            {
                printf("WS2812_Prime_Send: try again %d\n", phynum);
            }
        }
    }
}


PUBLIC void WS2812_Do_Send(void)
{
    if (Primed)
    {
//        printf("WS2812_Do_Send: %X\n", Primed);

        if (last_dma_completed_time)
        {
            if (get_absolute_time() < last_dma_completed_time)
            {
                printf("WS2812_Do_Send: Do Wait\n");
                while (get_absolute_time() < last_dma_completed_time)
                {
                    // Busy wait.
                }
                printf("WS2812_Do_Send: Done Wait\n");
            }
            last_dma_completed_time = 0;
        }

        dma_start_channel_mask(Primed);     // Start everybody.

        Primed = 0;

        last_dma_completed_time = get_absolute_time() + DMA_SEND_TIME;
    }
}

PUBLIC void WS2812_Init(void)
{
    uint offset = pio_add_program(pio0, &ws2812_program);
    offset = pio_add_program(pio1, &ws2812_program);

    WS2812_PHY* phy = WS2812_Phy;
    int i = 0;

    while (i < MAX_PHY)
    {
        PIO p = phy->pio = (i < 4) ? pio0 : pio1;
        int sm = phy->state_machine = i % 4;

        ws2812_program_init(p, sm, offset, i + WS2812_PIN, 800000, IS_RGBW);
        DMA_Init(phy);

        ++phy; ++i;
    }

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}


// EndFile: ws2812.c
