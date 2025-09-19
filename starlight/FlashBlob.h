// File: FlashBlob.h
#define FLASH_WRITE_SIZE 256                   // Minimum amount of data you can write at once.
#define FLASH_CHUNK_SIZE 4096                  // Minimum amount of data you can erase at once.
#define NUM_FLASH_CHUNK 256

#define MAX_BPAGE 16    // Valid bpages are 0 - 15.

#define BPAGE_VALID(bpage) ((bpage) >= 0 && (bpage) < MAX_BPAGE)

extern uint32_t* BPage_To_Address(int bpage);

extern bool BPage_Is_Blank(int bpage);
//
// Check is a flash page is actually blank.
// Return: true if page is blank.

extern uint16_t BPage_Blank_Pages(void);
//
// Scan through all pages and check for blanks.
// Return mask with 1 set for each blank page.

extern bool BPage_Erase_Page(int bpage);
//
// Erase a flash page.

extern void BPage_Erase_All_Pages(void);
//
// Erase every non-blank flash page.

extern bool BPage_Write_Blob(int bpage, uint8_t* buff);
extern bool BPage_Read_Blob(int bpage, uint8_t* buff);

extern bool BPage_Verify_Checksum(int bpage);

// End File: FlashBlob.h
