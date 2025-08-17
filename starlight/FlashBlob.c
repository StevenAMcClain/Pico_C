// File: FlashBLob.c

#include "common.h"
#include "FlashBlob.h"

#include <string.h>

#include <pico/flash.h>
#include <hardware/flash.h>

#include "blob.h"


typedef uint32_t FLASH_BASE;                   // Type for flash operations.

#define FLASH_BASE_ADDRESS 0x10000000          // Flash base address.
#define FLASH_SAVE_BASE 0x100000               // Flash save area base offset from flash base.

#define FLASH_WRITE_SIZE 256                   // Minimum amount of data you can write at once.
#define FLASH_CHUNK_SIZE 4096                  // Minimum amount of data you can erase at once.
#define NUM_FLASH_CHUNK 256

#define MAX_BPAGE 16    // Valid bpages are 0 - 15.

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


PRIVATE uint32_t bpage_to_offset(int bpage)
{
    return bpage * MAX_BLOB_SIZE;
}


PUBLIC uint32_t* bpage_to_address(int bpage)
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
    int count = MAX_BPAGE;

    while (count--)
    {
        if ( !(blanks & mask) )
        {
            bpage_erase_page(count);
        }
        mask <<= 1;
    }
}


PUBLIC void bpage_write_blob(int bpage, uint8_t* buff)
{
    size_t size = MAX_BLOB_SIZE;
    int rc = PICO_OK;
    int offset = FLASH_SAVE_BASE + bpage_to_offset(bpage);

    while (rc == PICO_OK && size)
    {
        uint32_t* params[] = { (void*)offset, (void*)buff};

        rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);

        buff += FLASH_PAGE_SIZE;  offset += FLASH_PAGE_SIZE;

        if (size > FLASH_PAGE_SIZE) size -= FLASH_PAGE_SIZE; else size = 0;
    }

    PRINTF("Flash Written: rc %d\n", rc);
}


PUBLIC void bpage_read_blob(int bpage, uint8_t* buff)
{
    uint32_t* ptr = bpage_to_address(bpage);
    memcpy(buff, ptr, MAX_BLOB_SIZE);
}


// End File: FlashBlob.c