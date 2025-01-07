// File: StarLight.c

#include "common.h"

#include "pico/stdlib.h"
#include "pico/flash.h"
#include <pico/multicore.h>

#include "hardware/flash.h"

#include <stdio.h>

#include "bluetooth_stdio.h"

#include "blob.h"
#include "debug.h"
#include "led.h"
#include "obled.h"
#include "parser.h"


PUBLIC uint32_t Debug_Mask = DEBUG_ALL;

//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};

uint32_t* start_ptr = (void*)0x10000000;
size_t size = 256;

#define PAGE_SIZE size

PUBLIC void mem_dumpx(void* ptr, size_t n)
{
	uint8_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			printf("\n%8.8X: ", bptr);
			count = 15;
		}
		else --count;

		printf("%2.2X ", *bptr++);
	}
	printf("\n");
}


extern uint32_t ADDR_PERSIST;                // Defined in 'pico_flash_region_protect.ld
void* FLASH_PERSIST_BASE = &ADDR_PERSIST;    // Persistent flash starts here.

#define FLASH_TARGET_OFFSET (0x100000)


// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) {
    uint32_t offset = (uint32_t)((uint32_t**)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}


void main(void)
{
    multicore_lockout_victim_init();

    stdio_init_all();           // Prepare stdio for use.

    // extern int __flash_binary_start;
    // printf("xxx %X\n", &__flash_binary_start);

    printf("\r\n\nStartlight %s %s %s: startup.\n", 
                         __VERSION__ , __TIME__, __DATE__);

    Start_BlueTooth_Core();     // Startup server on second core.
    LED_Init();                 // Prepare LED strings PIO driver for use.
    Blob_Init();                // Get blob engine ready to run.
    Start_Parser();             // Parse bluetooth stream..... Never return.
}


// EndFile: StarLight.c



//    Start_BlueTooth_Core();     // Startup server on second core.

    // printf("Start: persist = %X\n", FLASH_PERSIST_BASE);
    // while (1) ;

//     printf("Start\n");

//     while (1)
//     {
//         int ch = getchar_timeout_us(0);

//         if (ch != PICO_ERROR_TIMEOUT)
//         {
//             switch (ch)
//             {
//                 case '+':
//                 {
//                     start_ptr += PAGE_SIZE;
//                     mem_dumpx(start_ptr, size);
//                     break;
//                 }
//                 case '-':
//                 {
//                     start_ptr -= PAGE_SIZE;
//                     mem_dumpx(start_ptr, size);
//                     break;
//                 }
//                 case '/':
//                 {
//                     start_ptr += (PAGE_SIZE * 256);
//                     mem_dumpx(start_ptr, size);
//                     break;
//                 }
//                 case '*':
//                 {
//                     start_ptr -= (PAGE_SIZE  * 256);
//                     mem_dumpx(start_ptr, size);
//                     break;
//                 }
//                 case 'Q':
//                 {
//                     mem_dumpx(FLASH_PERSIST_BASE, 256);
//                     break;
//                 }
//                 case 'E':
//                 {
// multicore_lockout_start_blocking();                    
// call_flash_range_erase((void*)FLASH_TARGET_OFFSET);
// multicore_lockout_end_blocking();
// //                    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
//                     // printf("Flash Erased: rc %d\n", rc);
//                     printf("Flash Erased.\n");
//                     break;
//                 }
//                 case 'W':
//                 {
//                     uint8_t random_data[] = "Steven Was here.ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC1234DEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
//                     uint32_t* params[] = { (void*)FLASH_TARGET_OFFSET, (void*)&random_data};
// multicore_lockout_start_blocking();                    
// printf("start write.\n");
// call_flash_range_program(params);
// printf("finish write.\n");
// multicore_lockout_end_blocking();
//                     // int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);                    
//                     // printf("Flash Written: rc %d\n", rc);
//                     printf("Flash Written.\n");
//                     break;
//                 }
//             }
//         }
//     }
