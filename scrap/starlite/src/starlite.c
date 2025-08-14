// File: starlite.c

#include "common.h"

#include <stdio.h>
#include <string.h>

#include "btstdio.h"
#include "blob.h"
#include "debug.h"
#include "led.h"
#include "parser.h"


#include "shifter.h"


// PUBLIC uint32_t Debug_Mask = DEBUG_PRINTF | DEBUG_BLUETOOTH;  //DEBUG_ALL;
PUBLIC volatile uint32_t Debug_Mask = DEBUG_ALL;

// #define MAX_STACK_BUFF 100

// PRIVATE void zap_stack(void)
// {
//     char buff[MAX_STACK_BUFF];
//     memset(buff, 0xFF, sizeof(buff));
// }

// extern uint32_t __StackLimit;
// extern uint32_t __StackOneTop;
// extern uint32_t __StackTop;
// extern uint32_t __StackOneBottom;
// extern uint32_t __StackBottom;

PUBLIC void dump_stack()
{
    char ch;
    printf("Stackp: 0x%X\n", &ch);
    // printf("StackLimit 0x%X", __StackLimit);
    // printf(", StackOneTop 0x%X", __StackOneTop);
    // printf(", StackTop 0x%X", __StackTop);
    // printf(", StackOneBottom 0x%X", __StackOneBottom);
    // printf(", StackBottom 0x%X\n", __StackBottom);
    // // printf("StackLimit 0x%X, StackOneTop 0x%X, StackTop 0x%X, StackOneBottom 0x%X, StackBottom 0x%X\n",
    //     __StackLimit, __StackOneTop, __StackTop, __StackOneBottom, __StackBottom);

// extern void mem_dump_ints(void* ptr, size_t n);
//     char buff[MAX_STACK_BUFF];
//     mem_dump_ints(buff + MAX_STACK_BUFF - (sizeof(int) * size), size);
}


int main()
{
//    zap_stack();
    stdio_init_all();
//  dump_stack();

    printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth();     // Startup bluetooth stack (on second core).

    LED_Init();            // Prepare LED strings PIO driver for use.
    Blob_Init();           // Get blob engine ready to run.
    BtParser();            // Start parsing input characters ... never return.
}


// End File: starlite.c


//#define NUM_LEDS 32
// #define NUM_LEDS 310

// #define LEDS_PHY_IDX 0
// #define LEDS_PHY_MASK (1 << LEDS_PHY_IDX)
// #define LED_WHITE_VAL 20
// #define LED_STEP_TIME 80
// #define GPIO_INPUT_PIN 28

// int main()
// {
// //    zap_stack();
//     stdio_init_all();
//     // dump_stack();
//     gpio_init(GPIO_INPUT_PIN);
//     gpio_set_dir(GPIO_INPUT_PIN, false);
//     gpio_pull_up(GPIO_INPUT_PIN);

//     printf("\r\n\nStartlight %s %s %s: startup.\n", __VERSION__ , __TIME__, __DATE__);

// //    Start_BlueTooth_Core();     // Startup bluetooth stack (on second core).

//     LED_Init();                 // Prepare LED strings PIO driver for use.
// //    Blob_Init();                // Get blob engine ready to run.
// //    BtParser();                 // Start parsing input characters ... never return.

//     PHY_Set_led_count(LEDS_PHY_IDX, NUM_LEDS);
//     LED_All_RGB(LEDS_PHY_MASK, 0, 0, 0);
//     LEDS_Set_Phynum(LEDS_PHY_MASK);

//     while (1)
//     {
//         int val = gpio_get(GPIO_INPUT_PIN) ? 0 : LED_WHITE_VAL;

//         printf("val %d\n", val);

//         LED_Set_RGB(0, val, val, val);     // Set the first light.

//         // int i = NUM_LEDS + 1;
//         // while (i--)
//         {
//             LEDS_Do_Update();
//             sleep_ms(LED_STEP_TIME);
//             Command_Shift_LEDS(true, 1);
//         }
//         // sleep_ms(1000);
//         // LEDS_Do_Update();
//     }
// }

// End File: starlite.c
