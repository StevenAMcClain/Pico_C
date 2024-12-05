// File: StarLight.c

#include "Common.h"

#include <stdio.h>

#include "hardware/pio.h"

#include "bluetooth_stdio.h"

#include "obled.h"
#include "Blob.h"
#include "Led.h"
#include "Parser.h"

#include "ws2812.h"

//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};

//#define NUM_PIXELS 19       // This is the (initial) actual number LEDs on the string.

// int xxx[8]    = {600, 600, 600, 600, 600, 600, 600, 600};
// int xxx[8]    = {30, 30, 30, 30, 30, 30, 30, 30};

//extern void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);

void main(void)
{
    stdio_init_all();               // Prepare stdio for use.

    printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Server();

    LED_Init();           // Prepare LED strings driver for use.

    Blob_Init();
//printf("xxx %d\n", xxx[0]);

    PHY_Set_led_count(1, 19);
    LEDS_Set_Phynum(1);
    LEDS_All_Black();

    Start_Parser();  // Never returns.

    while (1);
   
//     LED Buff0[600] = {0};
//     LED Buff1[600] = {0};
//     LED Buff2[600] = {0};

//     Buff1[0].val = 0x04000000;
//     Buff1[1].val = 0x00040000;
//     Buff1[2].val = 0x00000400;
//     Buff1[3].val = 0x04000000;
//     Buff1[4].val = 0x00040000;
//     Buff1[5].val = 0x00000400;
//     Buff1[6].val = 0x04000000;
//     Buff1[7].val = 0x00040000;
//     Buff1[8].val = 0x00000400;
//     Buff1[9].val = 0x04000000;
//     Buff1[10].val = 0x00040000;
//     Buff1[11].val = 0x00000400;
//     Buff1[12].val = 0x04000000;
//     Buff1[13].val = 0x00040000;
//     Buff1[14].val = 0x00000400;
//     Buff1[15].val = 0x04000000;
//     Buff1[16].val = 0x00040000;
//     Buff1[17].val = 0x00000400;
//     Buff1[18].val = 0x04000000;
//     Buff1[19].val = 0x00040000;
//     Buff1[20].val = 0x00000400;
//     Buff1[21].val = 0x04000000;
//     Buff1[22].val = 0x00040000;
//     Buff1[23].val = 0x00000400;
//     Buff1[24].val = 0x04000000;
//     Buff1[25].val = 0x00040000;
//     Buff1[26].val = 0x00000400;
//     Buff1[27].val = 0x04000000;
//     Buff1[28].val = 0x00040000;
//     Buff1[29].val = 0x00000400;
//     Buff1[30].val = 0x04000000;
//     Buff1[31].val = 0x00040000;
//     Buff1[32].val = 0x00000400;
//     Buff1[33].val = 0x04000000;
//     Buff1[34].val = 0x00040000;
//     Buff1[35].val = 0x00000400;

//     Buff2[0].val = 0x00040000;
//     Buff2[1].val = 0x00000400;
//     Buff2[2].val = 0x04000000;
//     Buff2[3].val = 0x00040000;
//     Buff2[4].val = 0x00000400;
//     Buff2[5].val = 0x04000000;
//     Buff2[6].val = 0x00040000;
//     Buff2[7].val = 0x00000400;
//     Buff2[8].val = 0x04000000;
//     Buff2[9].val = 0x00040000;
//     Buff2[10].val = 0x00000400;
//     Buff2[11].val = 0x04000000;
//     Buff2[12].val = 0x00040000;
//     Buff2[13].val = 0x00000400;
//     Buff2[14].val = 0x04000000;
//     Buff2[15].val = 0x00040000;
//     Buff2[16].val = 0x00000400;
//     Buff2[17].val = 0x04000000;
//     Buff2[18].val = 0x00040000;
//     Buff2[19].val = 0x00000400;
//     Buff2[20].val = 0x04000000;
//     Buff2[21].val = 0x00040000;
//     Buff2[22].val = 0x00000400;
//     Buff2[23].val = 0x04000000;
//     Buff2[24].val = 0x00040000;
//     Buff2[25].val = 0x00000400;
//     Buff2[26].val = 0x04000000;
//     Buff2[27].val = 0x00040000;
//     Buff2[28].val = 0x00000400;
//     Buff2[29].val = 0x04000000;
//     Buff2[30].val = 0x00040000;
//     Buff2[31].val = 0x00000400;
//     Buff2[32].val = 0x04000000;
//     Buff2[33].val = 0x00040000;
//     Buff2[34].val = 0x00000400;
//     Buff2[35].val = 0x04000000;

//     WS2812_Set_Num_LEDS(0, xxx[0]);
//     WS2812_Set_Num_LEDS(1, xxx[1]);
//     WS2812_Set_Num_LEDS(2, xxx[2]);
//     WS2812_Set_Num_LEDS(3, xxx[3]);
//     WS2812_Set_Num_LEDS(4, xxx[4]);
//     WS2812_Set_Num_LEDS(5, xxx[5]);
//     WS2812_Set_Num_LEDS(6, xxx[6]);
//     WS2812_Set_Num_LEDS(7, xxx[7]);

//     while (1)
//     {
// //        WS2812_Send(0, (uint32_t*)Buff0);
// //      sleep_ms(1);
//         WS2812_Send(0, (uint32_t*)Buff1);
//         WS2812_Send(1, (uint32_t*)Buff1);
//     //   sleep_ms(1);
//         WS2812_Send(2, (uint32_t*)Buff2);
// //      sleep_ms(1);
// //        WS2812_Send(3, (uint32_t*)Buff0);
// //      sleep_ms(1);
//         WS2812_Send(3, (uint32_t*)Buff1);
//         WS2812_Send(4, (uint32_t*)Buff1);
// //      sleep_ms(1);
//         WS2812_Send(5, (uint32_t*)Buff2);
// //      sleep_ms(1);
// //        WS2812_Send(6, (uint32_t*)Buff0);
// //      sleep_ms(1);
//         WS2812_Send(6, (uint32_t*)Buff1);
//         WS2812_Send(7, (uint32_t*)Buff1);
// //        sleep_ms(1);
//         WS2812_Send(0, (uint32_t*)Buff1);
// //      sleep_ms(1);
//         WS2812_Send(1, (uint32_t*)Buff2);
// //      sleep_ms(1);
// //        WS2812_Send(2, (uint32_t*)Buff0);
// //      sleep_ms(1);
//         WS2812_Send(2, (uint32_t*)Buff1);
//         WS2812_Send(3, (uint32_t*)Buff1);
// //      sleep_ms(1);
//         WS2812_Send(4, (uint32_t*)Buff2);
//         WS2812_Send(5, (uint32_t*)Buff2);
// //      sleep_ms(1);
// //        WS2812_Send(5, (uint32_t*)Buff0);
// //      sleep_ms(1);
//         WS2812_Send(6, (uint32_t*)Buff1);
// //      sleep_ms(1);
//         WS2812_Send(7, (uint32_t*)Buff2);
//  //       sleep_ms(1);
//  //     sleep_ms(1);
//         WS2812_Send(0, (uint32_t*)Buff2);
//  //     sleep_ms(1);
//  //       WS2812_Send(1, (uint32_t*)Buff0);
//  //     sleep_ms(1);
//         WS2812_Send(1, (uint32_t*)Buff1);
//         WS2812_Send(2, (uint32_t*)Buff1);
//  //     sleep_ms(1);
//  //       WS2812_Send(1, (uint32_t*)Buff0);
//  //     sleep_ms(1);
//         WS2812_Send(3, (uint32_t*)Buff2);
//  //     sleep_ms(1);
//  //       WS2812_Send(4, (uint32_t*)Buff0);
//  //     sleep_ms(1);
//         WS2812_Send(4, (uint32_t*)Buff1);
//         WS2812_Send(5, (uint32_t*)Buff1);
//  //     sleep_ms(1);
//         WS2812_Send(6, (uint32_t*)Buff2);
//         WS2812_Send(7, (uint32_t*)Buff2);
//  //     sleep_ms(1);
//  //       WS2812_Send(7, (uint32_t*)Buff0);
//  //       sleep_ms(1);
//     }


// while(1)
// {
//     put_pixel(pio0, 0, 0);
//     put_pixel(pio0, 0, 0);
//     put_pixel(pio0, 0, 0);
//     put_pixel(pio0, 1, 0x000F0000);
//     put_pixel(pio0, 1, 0x00000F00);
//     put_pixel(pio0, 1, 0x0000000F);
//     put_pixel(pio0, 2, 0);
//     put_pixel(pio0, 2, 0);
//     put_pixel(pio0, 2, 0);
//     put_pixel(pio0, 3, 0x000F0000);
//     put_pixel(pio0, 3, 0x00000F00);
//     put_pixel(pio0, 3, 0x0000000F);


//     put_pixel(pio1, 0, 0);
//     put_pixel(pio1, 0, 0);
//     put_pixel(pio1, 0, 0);
//     put_pixel(pio1, 1, 0x000F0000);
//     put_pixel(pio1, 1, 0x00000F00);
//     put_pixel(pio1, 1, 0x0000000F);
//     put_pixel(pio1, 2, 0);
//     put_pixel(pio1, 2, 0);
//     put_pixel(pio1, 2, 0);
//     put_pixel(pio1, 3, 0x000F0000);
//     put_pixel(pio1, 3, 0x00000F00);
//     put_pixel(pio1, 3, 0x0000000F);


//     sleep_ms(1000);

//     put_pixel(pio0, 0, 0x000F0000);
//     put_pixel(pio0, 0, 0x00000F00);
//     put_pixel(pio0, 0, 0x0000000F);
//     put_pixel(pio0, 1, 0);
//     put_pixel(pio0, 1, 0);
//     put_pixel(pio0, 1, 0);
//     put_pixel(pio0, 2, 0x000F0000);
//     put_pixel(pio0, 2, 0x00000F00);
//     put_pixel(pio0, 2, 0x0000000F);
//     put_pixel(pio0, 3, 0);
//     put_pixel(pio0, 3, 0);
//     put_pixel(pio0, 3, 0);

//     put_pixel(pio1, 0, 0x000F0000);
//     put_pixel(pio1, 0, 0x00000F00);
//     put_pixel(pio1, 0, 0x0000000F);
//     put_pixel(pio1, 1, 0);
//     put_pixel(pio1, 1, 0);
//     put_pixel(pio1, 1, 0);
//     put_pixel(pio1, 2, 0x000F0000);
//     put_pixel(pio1, 2, 0x00000F00);
//     put_pixel(pio1, 2, 0x0000000F);
//     put_pixel(pio1, 3, 0);
//     put_pixel(pio1, 3, 0);
//     put_pixel(pio1, 3, 0);

//     sleep_ms(1000);
// }
    // Start_BlueTooth_Server();       // Start BlueTooth server on second core.
    // pico_led_init();                // Prepare pico led for use.
    // LED_Init(NUM_PIXELS);           // Prepare LED string driver for use.
    // Blob_Init();                    // Get Blob engine ready.
    // Start_Parser();                 // Start parsing BlueTooth characters.
}




// EndFile: StarLight.c