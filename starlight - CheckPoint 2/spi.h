// File: spi.h

#ifndef SPI_H
#define SPI_H

extern void Spi_Init(void);

extern uint16_t Spi_Read16(void);
extern uint32_t Spi_Read32(void);

extern bool Spi_Data_Ready(void);
extern void Spi_Process(void);

#endif  // SPI_H

// EndFile: spi.h
