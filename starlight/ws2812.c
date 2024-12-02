/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Modified by Steven A. McClain - definitely not backward compatible.
 */

#include "Common.h"

//#include <stdio.h>

#include "hardware/dma.h"
#include "pico/sem.h"
#include "hardware/pio.h"

#include "LED.h"
#include "ws2812.pio.h"

#include "ws2812.h"

#define IS_RGBW false

int do_send = 0;
int completed = 0;
int released = 0;


typedef struct
{
    PIO pio;                                            // PIO unit for this PHY.
    int state_machine;                                  // State machine number for PHY.
    int DMA_channel;                                    // DMA channel for PHY.
    struct semaphore reset_delay_complete_sem;          // posted when it is safe to output a new set of values.
    alarm_id_t reset_delay_alarm_id;                    // alarm handle for handling delay.

} WS2812_PHY;


PRIVATE WS2812_PHY WS2812_Phy[MAX_PHY] = {0};


PRIVATE int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data) 
{
    WS2812_PHY* phy = user_data;

    phy->reset_delay_alarm_id = 0;
    sem_release(&phy->reset_delay_complete_sem);
++released;

    return 0;    // no repeat
}


PRIVATE void __isr dma_complete_handler() 
{
    WS2812_PHY* phy = WS2812_Phy;
    int i = 0;

    int interrupts = dma_hw->ints0;
    int mask = 1;

    while (i++ < MAX_PHY)
    {
//printf("Interrupts: %X\n", interrupts);
        if (interrupts & mask) 
        {
            // when the dma is complete we start the reset delay timer
            //
            if (phy->reset_delay_alarm_id) cancel_alarm(phy->reset_delay_alarm_id);
            phy->reset_delay_alarm_id = add_alarm_in_us(400 + (i * 200), reset_delay_complete, phy, true);
++completed;
        }
        ++phy;  mask <<= 1;
    }

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
        MAX_LEDS,                       // Number of transfers; in this case each is 4 bytes.
        false                           // Don't start immediately.
    );

    dma_channel_set_irq0_enabled(chan, true);
}


PUBLIC void WS2812_Set_Num_LEDS(uint32_t phynum, size_t num_leds)
{
    if (phynum < MAX_PHY && num_leds <= MAX_LEDS)
    {
        WS2812_PHY* phy = WS2812_Phy + phynum;        
        dma_channel_set_trans_count(phy->DMA_channel, num_leds, false);
    }
}


PUBLIC void WS2812_Send(uint32_t phynum, uint32_t* buff)
{
    if (phynum < MAX_PHY)
    {
        WS2812_PHY* phy = WS2812_Phy + phynum;        
        sem_acquire_blocking(&phy->reset_delay_complete_sem);
        dma_channel_set_read_addr(phy->DMA_channel, buff, true);
++do_send;
    }
}


PUBLIC void WS2812_Init(void)
{
    uint offset = pio_add_program(pio0, &ws2812_program);
    offset = pio_add_program(pio1, &ws2812_program);

    WS2812_PHY* phy = WS2812_Phy;

    int i = 0;

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);

    while (i < MAX_PHY)
    {
        PIO p = phy->pio = (i < 4) ? pio0 : pio1;
        int sm = phy->state_machine = i % 4;

        ws2812_program_init(p, sm, offset, i + WS2812_PIN, 800000, IS_RGBW);

        sem_init(&phy->reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time

        DMA_Init(phy);
        ++phy; ++i;
    }

    irq_set_enabled(DMA_IRQ_0, true);
}


// EndFile: ws2812.c
