// File: spi_map.h

#ifndef SPI_MAP_H
#define SPI_MAP_H

#include "bvar.h"

extern void Spi_Map_Init(void);

extern void Spi_Map_Bind_Pot(int /*unit*/, BENG_VAR* /*var*/, FLOAT /*scale*/, FLOAT /*offset*/);

extern void Spi_Set_Button_Triggers(int /*unit*/, BENG_VAR* /*var*/, int /*pressed*/, int /*released*/);

extern void Spi_Map_Event(uint32_t /*val*/);

#endif  // SPI_MAP_H

// EndFile: spi_map.h
