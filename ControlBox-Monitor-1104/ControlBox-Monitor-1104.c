

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
