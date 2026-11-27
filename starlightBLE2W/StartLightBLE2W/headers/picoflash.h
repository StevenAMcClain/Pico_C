// File: picoflash.h

#ifndef PICOFLASH_H
#define PICOFLASH_H

#include <hardware/flash.h>


#define BAD_FLASH_OFFSET 0xFFFFFFFF
#define BAD_FLASH_ADDRESS ((uint8_t*)0xFFFFFFFF)
#define BAD_FLASH_SECTOR (MAX_FLASH_SECTOR_NUMBER + 1)

// extern const uint8_t __flash_rsvd_start[];     // 0x101FC000 (0)
// extern const uint8_t __flash_rsvd_end[];       // 0x10200000 (16384)

#define __flash_rsvd_start 0x101FC000
#define __flash_rsvd_end  0x10200000

#define FLASH_RESERVED_START  ((uint32_t)__flash_rsvd_start)
#define FLASH_RESERVED_END    ((uint32_t)__flash_rsvd_end)

#define FLASH_RESERVED_SIZE   (FLASH_RESERVED_END - FLASH_RESERVED_START)

typedef uint32_t flash_offset_t;               // Type for flash operations.

#define MAX_FLASH_SECTOR_NUMBER 4
#define MAX_FLASH_PAGE_NUMBER  (MAX_FLASH_SECTOR_NUMBER * (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE))

#define MAX_FLASH_OFFSET ((uint32_t)__flash_rsvd_end - XIP_BASE)


extern uint8_t* Flash_Page_Number_To_Address(size_t /*page_number*/);
extern uint8_t* Flash_Sector_Number_To_Address(size_t /*sector_number*/);

extern int Flash_Erase_Sector(size_t /*sector_number*/);
extern int Flash_Erase_All_Sectors(void);

extern int Flash_Write_Buff(size_t page_number, const uint8_t* /*buff*/, size_t /*buff_size_bytes*/);

extern bool Flash_Sector_Is_Blank(size_t /*sector_number*/);
//
// Check is a flash sector is actually blank.
// Return: true if whole sector is blank.

#endif //PICOFLASH_H

// End File: picoflash.h


// extern bool Flash_Read_Buff(size_t /*page_number*/, uint8_t* /*buff*/, size_t /*buff_size_bytes*/);

// #define FLASH_WRITE_SIZE 256                   // Minimum amount of data you can write at once.
// #define FLASH_CHUNK_SIZE 4096                  // Minimum amount of data you can erase at once.
// #define NUM_FLASH_CHUNK 256

// #define MAX_BPAGE 16    // Valid bpages are 0 - 15.

// #define BPAGE_VALID(bpage) ((bpage) >= 0 && (bpage) < MAX_BPAGE)

// extern uint32_t* BPage_To_Address(int bpage);

// extern bool BPage_Is_Blank(int bpage);
// //
// // Check is a flash page is actually blank.
// // Return: true if page is blank.

// extern uint16_t BPage_Blank_Pages(void);
// //
// // Scan through all pages and check for blanks.
// // Return mask with 1 set for each blank page.

// extern bool BPage_Erase_Page(int bpage);
// //
// // Erase a flash page.

// extern void BPage_Erase_All_Pages(void);
// //
// // Erase every non-blank flash page.

// extern bool BPage_Write_Blob(int bpage, uint8_t* buff);
// extern bool BPage_Read_Blob(int bpage, uint8_t* buff);

// extern bool BPage_Verify_Checksum(int bpage);

