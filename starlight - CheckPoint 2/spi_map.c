// File: spi_map.c

#include "common.h"
#include "spi_map.h"

#include <stdio.h>

#include "spi.h"

//#include <pico/stdlib.h>

typedef struct SPI_Pot_Thresh
{
    int old_value;
    int threshold;
    bool rising;        // True if rising, false if falling.

} SPI_POT_THRESH;


typedef struct SPI_Pot_Map
{
    BENG_VAR* var;

    FLOAT scale;
    FLOAT offset;

} SPI_POT_MAP;


#define MAX_POT_MAP 16
SPI_POT_MAP Spi_Pot_Maps[MAX_POT_MAP] = {0};


PUBLIC void Spi_Map_Bind_Pot(BENG_VAR* var, FLOAT scale, FLOAT offset)
{

}

#define SPI_DEVICE_BUTTON        2
#define SPI_DEVICE_POTENTIOMETER 4
#define SPI_DEVICE_ENCODER       8

PUBLIC void Spi_Map_Event(uint32_t raw_val)
{
    int device = (raw_val >> 28) & 0xF;
    int unit   = (raw_val >> 24) & 0xF;
    int value  =  raw_val & 0x00FFFFFF;

    //    printf("Spi_Map_Event: raw_val %x, dev %d, unit %d, val %d\n", raw_val, device, unit, value);

    if (unit)
    {
        --unit;

        if (device == SPI_DEVICE_POTENTIOMETER)
        {
            BENG_VAR* var = Spi_Pot_Maps[unit].var;

            if (var)
            {
                if (unit == 0)
                {
                    BVar_Set_uint(var, value >> 2); 
                }
                else if (unit == 1) 
                {
                    BVar_Set_uint(var, 10000 + (value * 100)); 
                }
            }
        }
    }
}

#include "bvar_internal.h"

PUBLIC void Spi_Map_Init()
{
    BENG_VAR* var = BVar_Find_Internal_By_Index(IVX_BRIGHT);
    Spi_Pot_Maps[0].var    = var;
    Spi_Pot_Maps[0].offset = 0;
    Spi_Pot_Maps[0].scale  = 0.25;

    var = BVar_Find_Internal_By_Index(IVX_TIK1);
    Spi_Pot_Maps[1].var    = var;
    Spi_Pot_Maps[1].offset = 10000;
    Spi_Pot_Maps[1].scale  = 100;
}


// EndFile: spi_map.c
