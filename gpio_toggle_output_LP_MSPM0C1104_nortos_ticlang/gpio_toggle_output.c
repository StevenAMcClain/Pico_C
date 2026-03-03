

#include "ti_msp_dl_config.h"

#define PUBLIC
#define PRIVATE static

#include <stdio.h>

/* This results in approximately 0.5s of delay assuming 24MHz CPU_CLK */
#define DELAY (12000000)

#define SPI_PACKET_SIZE (2)

#define BUFF_SIZE 50



PRIVATE void putstr(char* str)
{
    while (*str)
    {
        DL_UART_transmitDataBlocking(UART_0_INST, *str++);
    }
}


PRIVATE uint32_t fix_number(uint16_t* ptr)
{
    return (ptr[0] << 16) + ptr[1];
}


PRIVATE void print_one(int x)
{
    char buff[BUFF_SIZE];
    int d = (x >> 28) & 0xF;        // Extract Device Number
    int u = (x >> 24) & 0xF;        // Extract Unit Number
    int v = x & 0x00FFFFFF;         // Extract Value.

    sprintf(buff, ": 0x%x = d%d, u%d, v%d\r\n", x, d, u, v);
    putstr(buff);
}


//    uint32_t* nptr = (uint32_t*)ptr;
//    int i = SPI_PACKET_SIZE;
//    putstr("print: ");
//
//    while (i--)
//    {
//        sprintf(buff, "0x%x ", *ptr++);
//        putstr(buff);
//    }
//    putstr("\r\n");


PRIVATE void poll_spi()
{
    /* Data for SPI to transmit */
    PRIVATE uint8_t gTxPacket[] = {0x1, 0x2, 0x3, 0x4};

    /* Data for SPI to receive */
    uint16_t gRxPacket[SPI_PACKET_SIZE];

    /*
     * Fill FIFO with data.
     * Note that transactions are initiated by the Controller, so this function
     * only fills the buffer and the Peripheral device will send this data when
     * requested by the Controller.
     */
    DL_SPI_fillTXFIFO8(SPI_0_INST, &gTxPacket[0], SPI_PACKET_SIZE);

    /*
     * Wait to receive data from the SPI Controller
     * This loop expects SPI_PACKET_SIZE bytes
     */
    for (uint8_t i = 0; i < SPI_PACKET_SIZE; i++) {
        gRxPacket[i] = DL_SPI_receiveDataBlocking16(SPI_0_INST);
    }

    /*
     * Wait until all bytes written to TX FIFO are sent after a successful
     * read transfer initiated by the Controller.
     */
    while (!DL_SPI_isTXFIFOEmpty(SPI_0_INST))
        ;

    int v = fix_number(gRxPacket);
    print_one(v);
}


int main(void)
{
    SYSCFG_DL_init();

    putstr("\r\nUSBSPI V1.0 Start\r\n");

    while (1) { poll_spi(); }
}


// EndFile: SPIUSB.c



#ifdef COMMENT
/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"

/* This results in approximately 0.5s of delay assuming 24MHz CPU_CLK */
#define DELAY (12000000)

int main(void)
{
    /* Power on GPIO, initialize pins as digital outputs */
    SYSCFG_DL_init();

    /* Default: LED is off */
    DL_GPIO_setPins(
        GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_TEST_PIN);

    while (1) {
        /*
         * Call togglePins API to flip the current value of the LED. This
         * API causes the corresponding HW bits to be flipped by the GPIO HW
         * without need for additional R-M-W cycles by the processor.
         */
        delay_cycles(DELAY);
        DL_GPIO_togglePins(GPIO_LEDS_PORT,
            GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_TEST_PIN);
    }
}

#endif // COMMENT
