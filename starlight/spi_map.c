// File: spi_map.c

#include "common.h"
#include "spi_map.h"

#include <stdio.h>

#include "beng.h"
#include "spi.h"

//#include <pico/stdlib.h>


typedef struct SPI_Button
{
    BENG_VAR* var;

    int pressed_event;
    int released_event;

} SPI_BUTTON_MAP;


typedef struct SPI_Pot_Thresh
{
    int old_value;
    int threshold;
    bool rising;        // True if rising, false if falling.

    int trigger;        // Trigger number for this event.

} SPI_POT_THRESH;


typedef struct SPI_Pot_Map
{
    BENG_VAR* var;

    FLOAT scale;
    FLOAT offset;

} SPI_POT_MAP;


#define MAX_POT_MAP 16
SPI_POT_MAP Spi_Pot_Maps[MAX_POT_MAP] = {0};

SPI_BUTTON_MAP Spi_Buton_Maps[MAX_POT_MAP] = {0};


PUBLIC void Spi_Map_Bind_Pot(int unit, BENG_VAR* var, FLOAT scale, FLOAT offset)
{

}


PUBLIC void Spi_Set_Button_Triggers(int unit, BENG_VAR* var, int pressed, int released)
{

}


#define SPI_DEVICE_BUTTON        2
#define SPI_DEVICE_POTENTIOMETER 4
#define SPI_DEVICE_ENCODER       8

#define SPI_DEVICE_BUTTON_PRESSED 1
#define SPI_DEVICE_BUTTON_RELEASED 2


PUBLIC void Spi_Map_Event(uint32_t raw_val)
{
    int device = (raw_val >> 28) & 0xF;
    int unit   = (raw_val >> 24) & 0xF;
    int value  =  raw_val & 0x00FFFFFF;

    //    printf("Spi_Map_Event: raw_val %x, dev %d, unit %d, val %d\n", raw_val, device, unit, value);

    if (unit)
    {
        --unit;

        switch (device)
        {
            case SPI_DEVICE_POTENTIOMETER:
            {
                SPI_POT_MAP* pot = &Spi_Pot_Maps[unit];
                BENG_VAR* var = pot->var;

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
                break;
            }
            case SPI_DEVICE_BUTTON:
            {
                int trig = 0;
                SPI_BUTTON_MAP* button = &Spi_Buton_Maps[unit];

                if (value == SPI_DEVICE_BUTTON_PRESSED)
                {
                    trig = button->pressed_event;
                }
                else if (value == SPI_DEVICE_BUTTON_RELEASED)
                {
                    trig = button->released_event;
                }

                if (trig) { Blob_Trigger(0, trig); }

                if (button->var)
                {
                    BVar_Set_uint(button->var, value); 
                }
                break;
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

    Spi_Buton_Maps[0].pressed_event = 1;        // Set trigger events.
    Spi_Buton_Maps[0].released_event = 2;
    Spi_Buton_Maps[1].pressed_event = 3;
    Spi_Buton_Maps[1].released_event = 4;
    Spi_Buton_Maps[2].pressed_event = 5;
    Spi_Buton_Maps[2].released_event = 6;
    Spi_Buton_Maps[3].pressed_event = 7;
    Spi_Buton_Maps[3].released_event = 8;
    Spi_Buton_Maps[4].pressed_event = 9;
    Spi_Buton_Maps[4].released_event = 10;
}


// EndFile: spi_map.c
