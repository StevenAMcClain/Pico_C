// File: StarLight.c

#include "common.h"

#include "pico/stdlib.h"
#include <pico/multicore.h>
   
#include "pico/flash.h"
//#include "hardware/flash.h"

#include <stdio.h>

// #include "btstdio.h"
#include "blob.h"
#include "debug.h"
#include "led.h"
//#include "obled.h"
// #include "parser.h"

extern void Start_Parser(void);

//#include <hardware/pio.h>  //TEST
//extern void mem_dump(void* ptr, size_t n);

// PUBLIC uint32_t Debug_Mask = DEBUG_PRINTF | DEBUG_BLUETOOTH;  //DEBUG_ALL;
PUBLIC uint32_t Debug_Mask = DEBUG_ALL;

PUBLIC void _Printf(const char *fmt, ...) 
{
    if (Debug_Mask & DEBUG_PRINTF)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

//#define COUNT 100000000

extern void mem_dump_p(int (*p)(const char * restrict, ...), void* ptr, size_t n);
#define mem_dump_printf(ptr, size) mem_dump_p(PRINTF, ptr, size);

PRIVATE void Start_BlueTooth(void)
{
    extern void BlueTooth_Server(void);
    flash_safe_execute_core_init();
    BlueTooth_Server();
}

PUBLIC void main(void)
{
    stdio_init_all();           // Prepare stdio for use.

    PRINTF("\r\n\nStartlight %s %s %s: startup.\n", 
                            __VERSION__ , __TIME__, __DATE__);

#ifdef COMMENT
    PRINTF("PIO0 - Before\r\n");    mem_dump_printf(pio0, sizeof(pio_hw_t));
    PRINTF("PIO1 - Before\r\n");    mem_dump_printf(pio1, sizeof(pio_hw_t));
                                                               
    Start_BlueTooth_Core();     // Startup server on second core.

    PRINTF("PIO0 - After\r\n");    mem_dump_printf(pio0, sizeof(pio_hw_t));
    PRINTF("PIO1 - After\r\n");    mem_dump_printf(pio1, sizeof(pio_hw_t));

    int c = COUNT;
    while (1)
    {
        int i = BlueTooth_TryGetChar();
        if (i != PICO_ERROR_TIMEOUT)
        {
            PRINTF("Got '%c'\n", i);
            if (i == 'z') break;
        }
        if (--c == 0)
        {
            PRINTF(".\n");
            c = COUNT;
        }
    }

    PRINTF("PIO0 - Afterz\r\n");    mem_dump_printf(pio0, sizeof(pio_hw_t));
    PRINTF("PIO1 - Afterz\r\n");    mem_dump_printf(pio1, sizeof(pio_hw_t));

    while(1);
#endif

//    #ifdef COMMENT
    LED_Init();                 // Prepare LED strings PIO driver for use.
    Blob_Init();                // Get blob engine ready to run.


//extern void bpage_erase_page(int bpage);
//bpage_erase_page(12);

    uint16_t bpage_blank_pages();
    uint16_t blanks = bpage_blank_pages();
    printf("Blanks = %04.4X\n", blanks);

    multicore_lockout_victim_init();
    multicore_launch_core1(SECOND_CORE_MAIN);   // This is the main for the second core.

    FIRST_CORE_MAIN();                     // This is the main for the first core.
//    #endif
}


// EndFile: StarLight.c
