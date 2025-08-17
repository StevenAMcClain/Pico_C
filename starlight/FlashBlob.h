// File: FlashBlob.h

extern uint32_t* bpage_to_address(int bpage);

extern bool bpage_is_blank(int bpage);
//
// Check is a flash page is actually blank.
// Return: true if page is blank.

extern uint16_t bpage_blank_pages(void);
//
// Scan through all pages and check for blanks.
// Return mask with 1 set for each blank page.

extern void bpage_erase_page(int bpage);

extern void bpage_erase_all_pages(void);
//
// Erase each non-blank flash page.

extern void bpage_write_blob(int bpage, uint8_t* buff);
extern void bpage_read_blob(int bpage, uint8_t* buff);

// End File: FlashBlob.h
