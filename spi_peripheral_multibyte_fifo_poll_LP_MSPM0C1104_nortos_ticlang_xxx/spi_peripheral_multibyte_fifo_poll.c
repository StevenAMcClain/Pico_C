/*
 * Copyright (c) 2020, Texas Instruments Incorporated
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

#include <stdio.h>
/*
 * Number of bytes for SPI packet size
 * The packet will be transmitted by the SPI peripheral.
 * This example uses FIFOs with polling, and the maximum FIFO size is 4.
 * Refer to interrupt examples to handle larger packets.
 */
#define SPI_PACKET_SIZE (4)

/* Data for SPI to transmit */
uint8_t gTxPacket[SPI_PACKET_SIZE] = {0x1, 0x2, 0x3, 0x4};

char test[] = "Steven\r\n";
#define UART_PACKET_SIZE (sizeof(test)-1)

volatile uint8_t gURxPacket[UART_PACKET_SIZE];

/* Data for SPI to receive */
volatile uint8_t gRxPacket[SPI_PACKET_SIZE];


void putstr(char* str)
{
    while (*str)
    {
        DL_UART_transmitDataBlocking(UART_0_INST, *str++);
    }
}

#define BUFF_SIZE 50

void print_one(int x)
{
    char buff[BUFF_SIZE];
    int d = (x >> 28) & 0xF;
    int u = (x >> 24) & 0xF;
    int v = x & 0x00FFFFFF;

    sprintf(buff, "0x%x : d%d, u%d, v%d\r\n", x, d, u, v);
    putstr(buff);
}


volatile int x=033;

volatile int qq = 1000000;

int main(void)
{
    SYSCFG_DL_init();

    /* Set LED to indicate start of transfer */
//    DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_TEST_PIN);

    int val = 0x12000000;

while (1)
{
    print_one(val++);
    for (int i=0; i < qq; ++i) { x += 1; }
}

    /*
     * Wait to receive the UART data
     * This loop expects UART_PACKET_SIZE bytes
     */
    for (uint8_t i = 0; i < UART_PACKET_SIZE; i++) {
        gURxPacket[i] = DL_UART_receiveDataBlocking(UART_0_INST);
    }


    /*
     * Fill FIFO with data.
     * Note that transactions are initiated by the Controller, so this function
     * only fills the buffer and the Peripheral device will send this data when
     * requested by the Controller.
     */
//    DL_SPI_fillTXFIFO8(SPI_0_INST, &gTxPacket[0], SPI_PACKET_SIZE);


    /*
     * Wait to receive data from the SPI Controller
     * This loop expects SPI_PACKET_SIZE bytes
     */
//    for (uint8_t i = 0; i < SPI_PACKET_SIZE; i++) {
//        gRxPacket[i] = DL_SPI_receiveDataBlocking8(SPI_0_INST);
//    }

    /*
     * Wait until all bytes written to TX FIFO are sent after a successful
     * read transfer initiated by the Controller.
     */
//    while (!DL_SPI_isTXFIFOEmpty(SPI_0_INST))
//        ;

    /*
     * If this example is used with the spi_controller_multibyte_fifo_poll
     * example, the expected data to receive in gRxPacket is
     * {'M', 'S', 'P', '!'}.
     */

    /* If write and read were successful, toggle LED */
    while (1) {
//        DL_GPIO_togglePins(GPIO_LEDS_PORT,
//            GPIO_LEDS_USER_LED_1_PIN | GPIO_LEDS_USER_TEST_PIN);
//        delay_cycles(16000000);
    }
}

