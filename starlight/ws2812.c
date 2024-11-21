/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Modified by Steven A. McClain - definatly not backward compatible.
 */

#include "Common.h"

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

//#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pico/sem.h"
#include "hardware/pio.h"

#include "LED.h"
#include "ws2812.pio.h"

#include "ws2812.h"

#define IS_RGBW false

PRIVATE int DMA_Channel = 0;
PRIVATE dma_channel_config c = {0};
#define DMA_CHANNEL_MASK (1u << DMA_Channel)

// posted when it is safe to output a new set of values
PRIVATE struct semaphore reset_delay_complete_sem;

// alarm handle for handling delay
PRIVATE alarm_id_t reset_delay_alarm_id;


PRIVATE int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data) {
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);

    return 0;    // no repeat
}


PRIVATE void __isr dma_complete_handler() 
{
    if (dma_hw->ints0 & DMA_CHANNEL_MASK) {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;

        // when the dma is complete we start the reset delay timer
        if (reset_delay_alarm_id) cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}


PRIVATE void DMA_Init(PIO pio, size_t count)
{
    // Get a free channel, panic() if there are none
    DMA_Channel = dma_claim_unused_channel(true);
    
    c = dma_channel_get_default_config(DMA_Channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, 0, true));
    
    dma_channel_configure(
        DMA_Channel,    // Channel to be configured
        &c,             // The configuration we just created
        &pio->txf[0],   // The initial write address
        0,              // The initial read address
        count,          // Number of transfers; in this case each is 4 bytes.
        false           // Don't start immediately.
    );

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_Channel, true);
    irq_set_enabled(DMA_IRQ_0, true);
}


PUBLIC void sw2812_Set_Num_LEDS(size_t num_leds)
{
    dma_channel_set_trans_count(DMA_Channel, num_leds, false);
}


PUBLIC void sw2812_Send(uint32_t* buff)
{
    sem_acquire_blocking(&reset_delay_complete_sem);
    dma_channel_set_read_addr(DMA_Channel, buff, true);
}


PUBLIC void sw2812_Init(size_t num_leds)
{
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    DMA_Init(pio, num_leds);
}


// EndFile: ws2812.c