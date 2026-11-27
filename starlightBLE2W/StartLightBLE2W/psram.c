// File: psram.c

#include "common.h"
#include "balloc.h"

#include <hardware/structs/xip.h>
#include <hardware/sync.h>

#include <pico/stdlib.h>


#define GPIO_PSRAM_CS 47    // Chip Select pin for PSRAM (GPIO 47 on Raspberry Pi Pico)


#ifdef PICO_USE_PSRAM
#define BALLOC_START ((uint8_t*)0x11000000)   // Start address of PSRAM
#define BALLOC_SIZE  (8 * 1024 * 1024)        // Size of balloc PSRAM (8 MB)
#define BALLOC_START_UNCACHED ((uint8_t*)0x15000000)   // Start address of PSRAM (uncached)
#elif defined(PICO_RP2040)
#define BALLOC_START ((uint8_t*)0x00000000)   // Start address of PSRAM  (Not correct for RP2040, but used for testing!!!!!!)
#define BALLOC_SIZE  (16 * 1024)               // Size of balloc PSRAM (16 KB)
#define BALLOC_START_UNCACHED BALLOC_START   // Start address of PSRAM (uncached)
#else
#define BALLOC_START ((uint8_t*)0x00000000)   // Start address of PSRAM  (Not correct for RP2350, but used for testing!!!!!!)
#define BALLOC_SIZE  (8 * 1024)               // Size of balloc PSRAM (8 KB)
#define BALLOC_START_UNCACHED BALLOC_START   // Start address of PSRAM (uncached)
#endif

#define BALLOC_TEST 0x55AA55AA

PUBLIC bool     PSRAM_Ready  = false;
PUBLIC balloc_h PSRAM_Balloc = NIL;


PRIVATE inline bool psram_ready(void) 
{
    volatile uint32_t *p = (uint32_t*)BALLOC_START_UNCACHED;
    uint32_t old = *p;
    *p = BALLOC_TEST;
    uint32_t v = *p;
    *p = old;
    return v == BALLOC_TEST;
}


PUBLIC balloc_h PSram_Init(void)
{
#ifdef PICO_USE_PSRAM
    gpio_set_function(GPIO_PSRAM_CS, GPIO_FUNC_XIP_CS1);    // 1. Configure the PSRAM Chip Select Pin (GPIO 47) for XIP CS1
    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;         // 2. Enable write access to the external XIP memory region (PSRAM)
#endif
    PSRAM_Ready = psram_ready();

    if (PSRAM_Ready)
    {
        PSRAM_Balloc = Balloc_Init(BALLOC_START, BALLOC_SIZE);
    }
    return PSRAM_Balloc;                                    // 3. Initialize the balloc heap inside PSRAM
}


PUBLIC bool Is_Psram_Ptr(const void* ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)BALLOC_START;
    uintptr_t end  = base + BALLOC_SIZE;

    return (p >= base) && (p < end);
}


// End of file: psram.c
