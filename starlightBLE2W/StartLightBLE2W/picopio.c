// File: picopio.c

#include "common.h"
#include "picopio.h"

#include <stdio.h>
#include <string.h>

#include <hardware/dma.h>

#include <pico/sem.h>
#include <pico/critical_section.h>


PRIVATE pio_handle_t  Pio_Handle[MAX_PIO_UNITS] = {0};   // Array of pio records.

PRIVATE volatile uint32_t DMA_Start_Mask = 0;            // One bit for every DMA channel that is primed.

// Guards read-modify-write access to DMA_Start_Mask from PIO_DMA_Writer_Prime()
// and PIO_DMA_Writer_Start(), which may be called from different cores/contexts.
PRIVATE critical_section_t DMA_Start_Mask_Crit_Sec;


typedef struct pio_memory
{
    uint16_t Program_Bytes[MAX_PIO_PROGRAM_INSTRUCTIONS];
    uint16_t High_Water;

} pio_memory_t;

PRIVATE pio_memory_t Pio_Memory[MAX_PIO_DEVICES] = {0};


PRIVATE PIO Pio_Unit_to_Device(pio_unit_index index)
{
#ifdef PICO_RP2350
    return (index < 4) ? pio0 : (index < 8) ? pio1 : pio2;
#else
    return (index < 4) ? pio0 : pio1;
#endif
}


PRIVATE pio_device_index Pio_Unit_to_Device_Index(pio_unit_index index)
{
    return index / 4;
}


PRIVATE pio_state_machine_index Pio_Unit_to_SM_Index(pio_unit_index index)
{
    return index % 4;
}


PRIVATE pio_program_offset buffer_find_offset(const uint16_t *haystack, size_t hay_len,
                                              const uint16_t *needle,   size_t nee_len)
//
// Returns the offset where `needle` first appears inside `haystack`,
// or BAD_PIO_PROGRAM_OFFSET if not found.
{
    // empty needle or  cannot fit, is bad. 
    if ((nee_len == 0) || (nee_len > hay_len)) { return BAD_PIO_PROGRAM_OFFSET; }

    size_t limit = hay_len - nee_len + 1;

    for (size_t i = 0; i < limit; ++i)
    {
        bool match = true;

        for (size_t j = 0; j < nee_len; ++j)
        {
            if (haystack[i + j] != needle[j])
            {
                match = false;
                break;
            }
        }

        if (match) { return (pio_program_offset)i; }    // found at offset i
    }
    return BAD_PIO_PROGRAM_OFFSET;       // not found
}


/**
 * Load_Pio_Program
 *
 * Loads a PIO program into the specified PIO device, with software-side
 * tracking of instruction memory to avoid duplicate loads.
 *
 * Behavior:
 *  - Checks whether the program's instruction sequence already exists in the
 *    shadow copy of the PIO instruction memory (Pio_Memory[]). If found,
 *    returns the existing offset without touching hardware.
 *
 *  - If not found, calls pio_add_program() to load the program into the actual
 *    PIO instruction memory. The returned offset is then mirrored into the
 *    shadow memory so future loads can detect and reuse it.
 *
 *  - Updates High_Water to track the highest instruction index used by this
 *    PIO device. This is useful for diagnostics or future allocation logic.
 *
 * Constraints:
 *  - The caller must ensure pio_device_index is valid (0..MAX_PIO_DEVICES-1).
 *  - The program must fit within the 32‑instruction PIO memory region.
 *  - Only exact instruction-sequence matches are detected; this is not a
 *    free‑space allocator and does not prevent overlapping programs.
 *
 * Returns:
 *  - >= 0 : The instruction offset where the program resides in PIO memory.
 *  - -1   : The program could not be found or loaded.
 */
PRIVATE pio_program_offset PIO_Load_Program(pio_device_index dev, const pio_program_t *program)
{
    // Returned program offset inside the PIO instruction memory.
    // -1 means "not found / not loaded".
    pio_program_offset offset = BAD_PIO_PROGRAM_OFFSET;

    // Ensure the PIO index is valid for this device (RP2040 = 2, RP2350 = 3).
    if (dev < MAX_PIO_DEVICES)
    {
        // Local pointer to the shadow copy of this PIO's instruction memory.
        pio_memory_t* pio_memory = &Pio_Memory[dev];

        // Pointer to the program's 16‑bit instructions.
        const uint16_t* instructions = program->instructions;

        // Number of instructions in the program.
        uint8_t length = program->length;

        // Try to find an existing copy of this program in our shadow memory.
        // This allows reusing already‑loaded programs without re‑adding them.
        offset = buffer_find_offset(pio_memory->Program_Bytes,
                                    pio_memory->High_Water,
                                    instructions,
                                    length);

        // If not found, we must load the program into the actual PIO block.
        if (offset == BAD_PIO_PROGRAM_OFFSET)
        {
            // Select the correct PIO hardware block (pio0/pio1/pio2).
            PIO pio_device = Pio_Unit_to_Device(dev);

            if (pio_can_add_program(pio_device, program))
            {
                // Ask the SDK to allocate space and load the program into hardware.
                // Returns the instruction offset inside the PIO's 32‑instruction memory.
                offset = pio_add_program(pio_device, program);

                // If the program fits inside our shadow memory, store a copy.
                // This keeps our software-side representation in sync with hardware.
                if (offset >= 0 && offset <= (MAX_PIO_PROGRAM_INSTRUCTIONS - length))
                {
                    // Copy the program instructions into our shadow memory.
                    memcpy(pio_memory->Program_Bytes + offset,
                        instructions,
                        length * sizeof(uint16_t));

                    // Track the highest instruction index used so far.
                    pio_memory->High_Water = offset + length;
                }
            }
        }
    }
    return offset;    // Return the instruction offset (or -1 if not found / not loaded).
}


PRIVATE bool PIO_Claim_Unit(pio_unit_index unit)
{
    bool result = false;

    if (unit >= 0 && unit < MAX_PIO_UNITS)
    {
        pio_handle_t* pio = &Pio_Handle[unit];

        if (!pio->assigned)
        {
            pio_state_machine_index state_machine = pio->state_machine;
            PIO pio_device = pio->pio_device;

            bool is_enabled = (pio_device->ctrl & (1u << state_machine)) != 0;
            bool is_claimed = pio_sm_is_claimed(pio_device, state_machine);

                 if (is_enabled) { printf("\n\n *** WS2812_Init: already enabled pio %d, sm %d\n\n\n", pio->unit, state_machine); }
            else if (is_claimed) { printf("\n\n *** WS2812_Init: already claimed pio %d, sm %d\n\n\n", pio->unit, state_machine); }
            else
            {
                pio_sm_claim(pio_device, state_machine);
                pio->assigned = true;
                result = true;
            }
        }
    }
    return result;
}


/**
 * PIO_Find_Free
 *
 * Finds (and claims) a PIO unit that can run the given program, reusing an
 * already-loaded copy of the program where possible.
 *
 * Behavior:
 *  - Pass 1: for each PIO device, checks the shadow memory for an existing
 *    copy of the program. If found, tries to claim a free unit on that same
 *    device so the program can be reused without touching PIO hardware.
 *  - Pass 2: if no device already has the program loaded (or none of that
 *    device's units were free), finds a device with enough instruction-memory
 *    headroom for the program, claims a free unit on it, and loads the
 *    program there via PIO_Load_Program().
 *
 * Returns:
 *  - A pointer to the claimed pio_handle_t on success.
 *  - NULL if no unit could be claimed, or the program doesn't fit anywhere.
 */
PUBLIC pio_handle_t* PIO_Allocate(const pio_program_t *program)
{
    const uint16_t* instructions = program->instructions;
    uint8_t         length       = program->length;
    pio_handle_t*   pio   = NULL;

    // Pass 1: program already loaded somewhere? Try to claim a free unit on
    // that device so we can reuse it without touching PIO hardware.
    for (pio_device_index dev = 0; dev < MAX_PIO_DEVICES && pio == NULL; ++dev)
    {
        pio_memory_t* pio_memory = &Pio_Memory[dev];

        pio_program_offset offset = buffer_find_offset(pio_memory->Program_Bytes,
                                                        pio_memory->High_Water,
                                                        instructions,
                                                        length);
        if (offset != BAD_PIO_PROGRAM_OFFSET)
        {
            pio_unit_index first = dev * PIO_UNITS_PER_DEVICE;
            pio_unit_index last  = first + PIO_UNITS_PER_DEVICE;

            for (pio_unit_index idx = first; idx < last; ++idx)
            {
                if (PIO_Claim_Unit(idx))
                {
                    pio = &Pio_Handle[idx];
                    pio->offset = offset;
                    break;
                }
            }
        }
    }

    // Pass 2: not loaded anywhere (or no free unit where it was loaded).
    // Find a device with room for it, claim a free unit, and load it there.
    if (pio == NULL)
    {
        for (pio_device_index dev = 0; dev < MAX_PIO_DEVICES && pio == NULL; ++dev)
        {
            PIO pio_device = Pio_Unit_to_Device(dev * PIO_UNITS_PER_DEVICE);

            if (!pio_can_add_program(pio_device, program))
            {
                continue;   // Program doesn't fit in this device's instruction memory.
            }

            pio_unit_index first = dev * PIO_UNITS_PER_DEVICE;
            pio_unit_index last  = first + PIO_UNITS_PER_DEVICE;

            for (pio_unit_index idx = first; idx < last; ++idx)
            {
                if (PIO_Claim_Unit(idx))
                {
                    pio_program_offset offset = PIO_Load_Program(dev, program);

                    if (offset != BAD_PIO_PROGRAM_OFFSET)
                    {
                        pio = &Pio_Handle[idx];
                        pio->offset = offset;
                    }
                    // NOTE: if PIO_Load_Program() ever fails here despite
                    // pio_can_add_program() having said yes, the unit claimed
                    // by PIO_Setup_Pio() above is left claimed with pio_handle
                    // still NULL — there's currently no PIO_Release_Pio() to
                    // undo that claim. Flagging so it isn't a silent leak.
                    break;
                }
            }
        }
    }

    return pio;
}


PUBLIC void PIO_DMA_Writer_Init(pio_handle_t* pio_handle)
{
    static bool DMA_Start_Mask_Crit_Sec_Ready = false;

    if (!DMA_Start_Mask_Crit_Sec_Ready)
    {
        critical_section_init(&DMA_Start_Mask_Crit_Sec);
        DMA_Start_Mask_Crit_Sec_Ready = true;
    }

    // Get a free channel, panic() if there are none
    dma_channel_index chan = pio_handle->DMA_Write = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(chan);
    PIO pio_device = pio_handle->pio_device;

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio_device, pio_handle->state_machine, true));

    volatile void *write_addr = &pio_device->txf[pio_handle->state_machine];

    dma_channel_configure(chan,           // Channel to be configured
                          &cfg,           // The configuration we just created
                          write_addr,     // The initial write address
                          0,              // The initial read address
                          0,              // Number of transfers; in this case each is 4 bytes.
                          false );        // Don't start immediately.
}


/**
 * PIO_DMA_Writer_Prime
 *
 * Prepares a DMA channel for a PIO TX FIFO write operation by configuring
 * the transfer count and the source buffer address. This does not start
 * the DMA transfer; it only primes the channel so that a later call to
 * dma_start_channel_mask() can trigger one or more channels simultaneously.
 *
 * Parameters:
 *   chan      - DMA channel number previously configured for PIO TX writes.
 *   buff      - Pointer to the source buffer containing 32‑bit PIO words.
 *   num_leds  - Number of 32‑bit words to transfer (typically LED count).
 *
 * Behavior:
 *   - Sets the DMA transfer count to num_leds (each transfer is 32 bits).
 *   - Sets the DMA read address to buff.
 *   - Leaves the write address and DREQ configuration untouched (set earlier
 *     in PIO_DMA_Writer_Init).
 *
 * Notes:
 *   - This function does not validate buffer size; caller must ensure buff
 *     contains at least num_leds 32‑bit entries.
 *   - This function does not start the DMA; caller must invoke
 *     PIO_DMA_Writer_Start() or dma_start_channel_mask().
 *   - Access to DMA_Start_Mask is protected by a critical section so that
 *     concurrent priming (e.g. from both cores or an IRQ) can't lose an
 *     update to another in-flight read-modify-write.
 */
PUBLIC void PIO_DMA_Writer_Prime(dma_channel_index dma, uint32_t* buff, int num_leds)
{
    // Configure how many 32‑bit transfers the DMA will perform.
    dma_channel_set_trans_count(dma, num_leds, false);

    // Set the source buffer address for the DMA read side.
    // The write address (PIO TX FIFO) was already configured in Init.
    dma_channel_set_read_addr(dma, buff, false);

    critical_section_enter_blocking(&DMA_Start_Mask_Crit_Sec);
    DMA_Start_Mask |= (1 << dma);
    critical_section_exit(&DMA_Start_Mask_Crit_Sec);
}


PUBLIC void PIO_DMA_Writer_Start(void)
{
    uint32_t mask;

    critical_section_enter_blocking(&DMA_Start_Mask_Crit_Sec);
    mask = DMA_Start_Mask;
    DMA_Start_Mask = 0;
    critical_section_exit(&DMA_Start_Mask_Crit_Sec);

    // Atomically start all DMA channels specified in the mask.
    dma_start_channel_mask(mask);
}


PUBLIC void PicoPIO_Initialize(void)
//
// Called once by main to preapre everything.
{
    pio_handle_t*pio = Pio_Handle;
    size_t count = MAX_PIO_UNITS;
    pio_unit_index unit = 0;

    while (count--)
    {
        pio->unit = unit;
        pio->device = Pio_Unit_to_Device_Index(unit);
        pio->state_machine = Pio_Unit_to_SM_Index(unit);
        pio->pio_device = Pio_Unit_to_Device(unit);
        pio->assigned = pio->ready = false;
        pio->offset = BAD_PIO_PROGRAM_OFFSET;
        ++unit;  ++pio;
    }
}

// Endfile: picopio.c
