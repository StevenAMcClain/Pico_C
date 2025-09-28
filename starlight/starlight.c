// File: StarLight.c

#include "common.h"

#include <hardware/flash.h>

#include "pico/stdlib.h"
#include <pico/multicore.h>
   
#include "pico/flash.h"

#include <stdio.h>

#include "blob.h"
#include "beng.h"
#include "debug.h"
#include "led.h"
#include "parser.h"
#include "flashblob.h"


PUBLIC uint8_t uuid[8] = {0};

//#include <hardware/pio.h>  //TEST
//extern void mem_dump(void* ptr, size_t n);

// PUBLIC uint32_t Debug_Mask = DEBUG_PRINTF | DEBUG_BLUETOOTH;  //DEBUG_ALL;
PUBLIC uint32_t Debug_Mask = DEBUG_ALL;
//PUBLIC uint32_t Debug_Mask = 0;  //DEBUG_ALL;

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


// extern void mem_dump_p(int (*p)(const char * restrict, ...), void* ptr, size_t n);
// #define mem_dump_printf(ptr, size) mem_dump_p(PRINTF, ptr, size);


PRIVATE void Start_BlueTooth(void)
{
    extern void BlueTooth_Server(void);
    flash_safe_execute_core_init();
    BlueTooth_Server();
}


PUBLIC void main(void)
{
    stdio_init_all();           // Prepare stdio for use.

    flash_get_unique_id(uuid);

    PRINTF("\r\n\nStarlight %s %s %s: UUID:%02X%02X%02X%02X%02X%02X%02X%02X: startup.\n", 
                            __VERSION__ , __TIME__, __DATE__, 
                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7]);

    LED_Init();                 // Prepare LED strings PIO driver for use.
    Blob_Init();                // Get blob manager ready to run.
    Beng_Init();                // Get blob engine ready.

    multicore_lockout_victim_init();
    multicore_launch_core1(SECOND_CORE_MAIN);   // This is the main for the second core.

    sleep_ms(100);
    
                                                                                                                {
                                                                                                                    uint16_t blanks = BPage_Blank_Pages();
                                                                                                                    printf("Blanks = %04.4X\n", blanks);
                                                                                                                }
    FIRST_CORE_MAIN();                     // This is the main for the first core.
}


// EndFile: StarLight.c
