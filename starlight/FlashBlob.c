// File: FlashBLob.c

#include "common.h"
#include "FlashBlob.h"

//#include <stdio.h>

#include <pico/flash.h>
#include <hardware/flash.h>

#include "blob.h"

typedef uint32_t FLASH_BASE;                   // Type for flash operations.

#define FLASH_BASE_ADDRESS 0x10000000          // Flash base address.
#define FLASH_SAVE_BASE 0x100000               // Flash save area base offset from flash base.

#define FLASH_WRITE_SIZE 256                   // Minimum amount of data you can write at once.
#define FLASH_CHUNK_SIZE 4096                  // Minimum amount of data you can erase at once.
#define NUM_FLASH_CHUNK 256


//PRIVATE uint32_t* flash_base_ptr = (uint32_t*)0x10000000;

#define MAX_BPAGE_SIZE (MAX_BLOB_SIZE / sizeof(FLASH_BASE))


PRIVATE void call_flash_range_erase(void *param)
//
// This function will be called when it's safe to call flash_range_erase.
{
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_CHUNK_SIZE);
}

PRIVATE void call_flash_range_program(void *param) 
//
// This function will be called when it's safe to call flash_range_program.
{
    uint32_t offset = (uint32_t)((uint32_t**)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}


PRIVATE void do_erase(size_t offset)
{
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_SAVE_BASE + offset, UINT32_MAX);
    PRINTF("Flash Erased: rc %d\n", rc);
}

void do_write()
{
    uint8_t *random_data = "Steven Was here.ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC1234DEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uint32_t* params[] = { (void*)FLASH_SAVE_BASE, (void*)random_data};

    int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);                    

    PRINTF("Flash Written: rc %d\n", rc);
}


PRIVATE uint32_t bpage_to_offset(int bpage)
{
    return bpage * MAX_BLOB_SIZE;
}


PRIVATE uint32_t* bpage_to_address(int bpage)
{
    return (uint32_t*)(FLASH_BASE_ADDRESS + FLASH_SAVE_BASE + bpage_to_offset(bpage));
}


PUBLIC bool bpage_is_blank(int bpage)
//
// Check is a flash page is actually blank.
// Return: true if page is blank.
{
    uint32_t* ptr = bpage_to_address(bpage);
    size_t count = MAX_BPAGE_SIZE / sizeof(*ptr);

    while (count--)
    {
        if (*ptr++ != 0xFFFFFFFF)
        {
            return false;
        }
    }
    return true;
}


PUBLIC uint16_t bpage_blank_pages(void)
//
// Scan through all pages and check for blanks.
// Return mask with 1 set for each blank page.
{
    uint16_t result = 0;
    int count = 16;
    uint16_t bmask = 1 << 15;

    while (count--)
    {
        if (bpage_is_blank(count))
        {
            result |= bmask;
        }
        bmask >>= 1;
    }

    return result;
}


PUBLIC void bpage_erase_page(int bpage)
{
    uint32_t offset = bpage_to_offset(bpage);
    int count = MAX_BLOB_SIZE / FLASH_CHUNK_SIZE;

    while (count--)
    {
        do_erase(offset);
        offset += FLASH_CHUNK_SIZE;
    }
}


PUBLIC void bpage_erase_all_pages(void)
//
// Erase each non-blank flash page.
{
    uint16_t blanks = bpage_blank_pages();
    uint16_t mask = 1 << 15;
    int count = 16;

    while (count--)
    {
        if ( !(blanks & mask) )
        {
            bpage_erase_page(count);
        }
        mask <<= 1;
    }
}


//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};
// size_t size = 256;
// #define PAGE_SIZE size
// PUBLIC void mem_dumpx(void* ptr, size_t n)
// {
// 	uint8_t* bptr = ptr;
// 	uint8_t count = 0;

// 	while (n--)
// 	{
// 		if (!count)
// 		{
// 			PRINTF("\n%8.8X: ", bptr);
// 			count = 15;
// 		}
// 		else --count;

// 		PRINTF("%2.2X ", *bptr++);
// 	}
// 	PRINTF("\n");
// }

    // PRINTF("Start: persist = %X\n", FLASH_PERSIST_BASE);
    // while (1) ;

    // extern int __flash_binary_start;
    // PRINTF("xxx %X\n", &__flash_binary_start);

//                 case 'E':
//                 {
// multicore_lockout_start_blocking();                    
// call_flash_range_erase((void*)FLASH_TARGET_OFFSET);
// multicore_lockout_end_blocking();
// //                    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
//                     // PRINTF("Flash Erased: rc %d\n", rc);
//                     PRINTF("Flash Erased.\n");


//                 case 'W':
//                 {
//                     uint8_t random_data[] = "Steven Was here.ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC1234DEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
//                     uint32_t* params[] = { (void*)FLASH_TARGET_OFFSET, (void*)&random_data};
// multicore_lockout_start_blocking();                    
// PRINTF("start write.\n");
// call_flash_range_program(params);
// PRINTF("finish write.\n");
// multicore_lockout_end_blocking();
//                     // int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);                    
//                     // PRINTF("Flash Written: rc %d\n", rc);
//                     PRINTF("Flash Written.\n");


// End File: FlashBlob.c







// uint32_t* start_ptr = (void*)0x10000000;
// size_t size = 256;

// #define PAGE_SIZE size

// PUBLIC void mem_dumpx(void* ptr, size_t n)
// {
// 	uint8_t* bptr = ptr;
// 	uint8_t count = 0;

// 	while (n--)
// 	{
// 		if (!count)
// 		{
// 			PRINTF("\n%8.8X: ", bptr);
// 			count = 15;
// 		}
// 		else --count;

// 		PRINTF("%2.2X ", *bptr++);
// 	}
// 	PRINTF("\n");
// }


//__attribute__((section(".uninitialized_data")))
//int xxx[8] = {593, 594, 595, 596, 597, 598, 599, 600};

// extern int __flash_binary_start;
// PRINTF("xxx %X\n", &__flash_binary_start);

// extern uint32_t ADDR_PERSIST;                // Defined in 'pico_flash_region_protect.ld
// void* FLASH_PERSIST_BASE = &ADDR_PERSIST;    // Persistent flash starts here.

// #define FLASH_TARGET_OFFSET (0x100000)


// // This function will be called when it's safe to call flash_range_erase
// static void call_flash_range_erase(void *param) {
//     uint32_t offset = (uint32_t)param;
//     flash_range_erase(offset, FLASH_SECTOR_SIZE);
// }

// // This function will be called when it's safe to call flash_range_program
// static void call_flash_range_program(void *param) {
//     uint32_t offset = (uint32_t)((uint32_t**)param)[0];
//     const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
//     flash_range_program(offset, data, FLASH_PAGE_SIZE);
// }

//    Start_BlueTooth_Core();     // Startup server on second core.

    // PRINTF("Start: persist = %X\n", FLASH_PERSIST_BASE);
    // while (1) ;

//     PRINTF("Start\n");

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
//                     // PRINTF("Flash Erased: rc %d\n", rc);
//                     PRINTF("Flash Erased.\n");
//                     break;
//                 }
//                 case 'W':
//                 {
//                     uint8_t random_data[] = "Steven Was here.ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC1234DEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
//                     uint32_t* params[] = { (void*)FLASH_TARGET_OFFSET, (void*)&random_data};
// multicore_lockout_start_blocking();                    
// PRINTF("start write.\n");
// call_flash_range_program(params);
// PRINTF("finish write.\n");
// multicore_lockout_end_blocking();
//                     // int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);                    
//                     // PRINTF("Flash Written: rc %d\n", rc);
//                     PRINTF("Flash Written.\n");
//                     break;
//                 }
//             }
//         }
//     }
