// File: spi.c

#include "Common.h"

#include "Spi.h"

#include <stdio.h>

//#include <pico/stdlib.h>

#include "pico/stdlib.h"

#include "hardware/spi.h"

#include "spi_map.h"


#define SPI_DEVICE  spi1
#define SPI_SCK_PIN  10
#define SPI_TX_PIN   11
#define SPI_RX_PIN   12
#define SPI_CSN_PIN  13


PUBLIC void Spi_Init()
{
    spi_init(SPI_DEVICE, 1000 * 1000);
    spi_set_slave(SPI_DEVICE, true);

    spi_set_format(SPI_DEVICE, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);   // P10
    gpio_set_function(SPI_TX_PIN,  GPIO_FUNC_SPI);   // P11
    gpio_set_function(SPI_RX_PIN,  GPIO_FUNC_SPI);   // P12
    gpio_set_function(SPI_CSN_PIN, GPIO_FUNC_SPI);   // P13
}


PUBLIC uint16_t Spi_Read16(void) 
{
    uint8_t tx_buf[2] = {0x00, 0x00};  // Dummy bytes to generate clock
    uint8_t rx_buf[2];

    if (spi_write_read_blocking(SPI_DEVICE, tx_buf, rx_buf, 2) == 2)
    {
        return (rx_buf[0] << 8) | rx_buf[1];  // Combine MSB and LSB
    }
    return 0;
}


PUBLIC uint32_t Spi_Read32(void) 
{
    uint8_t tx_buf[4] = {0x55, 0xAA, 0xF0, 0xF0};  // Dummy bytes to generate clock
    uint8_t rx_buf[4] = {0};

    if (spi_write_read_blocking(SPI_DEVICE, tx_buf, rx_buf, 4) == 4)
    {
        return (rx_buf[0] << 24) | (rx_buf[1] << 16) | (rx_buf[2] << 8) | rx_buf[3];  // Combine MSB and LSB
    }
    return 0;
}


PUBLIC bool Spi_Data_Ready(void)
{
    return spi_is_readable(SPI_DEVICE);
}


PUBLIC void Spi_Process(void)
{
    uint32_t val = Spi_Read32();

    // printf("Spi_Process: val %x\n", val);

    Spi_Map_Event(val);
}


// EndFile: spi.c
