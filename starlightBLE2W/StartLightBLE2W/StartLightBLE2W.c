// File: starlightBLE.c

#include "common.h"

#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "balloc.h"
#include "BLECore.h"
#include "picopio.h"
#include "shell.h"
#include "spsc_queue.h"


PUBLIC uint32_t Debug_Mask = DEBUG_MASK_INITIAL;
PUBLIC uint32_t Debug_Level = DEBUG_LEVEL_INITIAL;

// #ifdef PICO_RP2350
// #include "pico/binary_info.h"

// bi_decl(bi_program_name("my_program"));
// bi_decl(bi_program_description("My RP2350 firmware"));
// #endif

  // Place this buffer in PSRAM (not in flash)
// __attribute__((section(".psram_data")))
//uint8_t __attribute__((section(".psram_data"))) big_buffer[1024 * 1024];   // 1 MB buffer in PSRAM
///uint32_t* big_buffer = (uint32_t*)0x11000000;   // 1 MB buffer in PSRAM


PUBLIC int main(void) 
{
    stdio_init_all();

    if (PSram_Init())
    {
        //    Debug_Pin_Initialize(2);

        #if LIB_PICO_STDIO_USB
        while (!stdio_usb_connected()) { sleep_ms(100); } sleep_ms(100);
        #endif 
        
        PicoPIO_Initialize();
        spsc_queue_init();

        multicore_lockout_victim_init();
        multicore_launch_core1(Shell);    // Startup the shell.

        BLECore_main();                   // Never returns.
    }
    
    return 0;
}


// End of file: starlightBLE.c
