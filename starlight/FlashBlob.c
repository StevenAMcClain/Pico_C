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


PRIVATE inline uint32_t bpage_to_offset(int bpage)
{
    return bpage * MAX_BLOB_SIZE;
}


PUBLIC inline uint32_t* BPage_To_Address(int bpage)
{
    return (uint32_t*)(FLASH_BASE_ADDRESS + FLASH_SAVE_BASE + bpage_to_offset(bpage));
}


PUBLIC bool BPage_Is_Blank(int bpage)
//
// Check is a flash page is actually blank.
// Return: true if page is blank.
{
   	if (BPAGE_VALID(bpage))
    {
        uint32_t* ptr = BPage_To_Address(bpage);
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
    return false;
}


PUBLIC uint16_t BPage_Blank_Pages(void)
//
// Scan through all pages and check for blanks.
// Return mask with 1 set for each blank page.
{
    uint16_t result = 0;
    int count = 16;
    uint16_t bmask = 1 << 15;

    while (count--)
    {
        if (BPage_Is_Blank(count))
        {
            result |= bmask;
        }
        bmask >>= 1;
    }

    return result;
}


PUBLIC bool BPage_Erase_Page(int bpage)
{
   	if (BPAGE_VALID(bpage))
    {
        uint32_t offset = bpage_to_offset(bpage);
        int count = MAX_BLOB_SIZE / FLASH_CHUNK_SIZE;

        while (count--)
        {
            do_erase(offset);
            offset += FLASH_CHUNK_SIZE;
        }
        return true;
    }
    return false;
}


PUBLIC void BPage_Erase_All_Pages(void)
//
// Erase each non-blank flash page.
{
    uint16_t blanks = BPage_Blank_Pages();
    uint16_t mask = 1 << 15;
    int count = MAX_BPAGE;

    while (count--)
    {
        if ( !(blanks & mask) )
        {
            BPage_Erase_Page(count);
        }
        mask <<= 1;
    }
}


PUBLIC bool BPage_Write_Blob(int bpage, uint8_t* buff)
{
   	if (BPAGE_VALID(bpage))
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
        return true;
    }
    return false;
}


PUBLIC bool BPage_Verify_Checksum(int bpage)
{
   	if (BPAGE_VALID(bpage))
    {
        return Blob_Verify_Checksum((uint8_t*)BPage_To_Address(bpage));
    }
    return false;
}


PUBLIC bool BPage_Read_Blob(int bpage, uint8_t* buff)
{
   	if (BPAGE_VALID(bpage))
    {
        uint32_t* ptr = BPage_To_Address(bpage);
        memcpy(buff, ptr, MAX_BLOB_SIZE);
        return true;
    }
    return false;
}



// End File: FlashBlob.c