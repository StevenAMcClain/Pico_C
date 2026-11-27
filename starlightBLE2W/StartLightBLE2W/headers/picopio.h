// File: picopio.h

#ifndef PICOPIO_H
#define PICOPIO_H

#include <hardware/pio.h>

#ifdef PICO_RP2350
#define MAX_PIO_UNITS 12
#define MAX_PIO_DEVICES 3
#else
#define MAX_PIO_UNITS 8
#define MAX_PIO_DEVICES 2
#endif

// Number of PIO units (state machines) per PIO device.
#define PIO_UNITS_PER_DEVICE 4

#define MAX_PIO_PROGRAM_INSTRUCTIONS 32

typedef int8_t pio_unit_index;                 // Uiniq index for every possible pio state machine RP2040(0-7) or RP2350(0-11)
typedef uint8_t pio_device_index;               // Index for the pio hardware device RP2040(0-1) or RP2350(0-2)
typedef uint8_t pio_state_machine_index;        // State machine on device (0-3)
typedef uint8_t dma_channel_index;              // DMA channel associated with write fifo.
typedef int8_t pio_program_offset;             // Offset into PIO program buffer (0-31)

#define BAD_PIO_PROGRAM_OFFSET ((pio_program_offset)(-1))
#define BAD_PIO_UNIT_INDEX ((pio_unit_index)(-1))


typedef struct pio_handle
{
    PIO pio_device;                                // PIO device pointer.

    pio_unit_index unit;                    // pio unit index.
    pio_device_index device;                // pio device index.
    pio_state_machine_index state_machine;  // State machine number.
    pio_program_offset offset;              // Offset for start of program.
    dma_channel_index DMA_Write;            // DMA channel.

    bool assigned;                          // true when pio unit has been assigned.
    bool ready;                             // true when pio is all setup.

} pio_handle_t;

// Prepare PIO system for use.
//
extern void PicoPIO_Initialize(void);

// Claims and prepares a PIO state machine for use based on a logical index.
//extern bool PIO_Claim_Unit(pio_unit_index index);

// Loads a PIO program into hardware (or reuses an existing load) and returns its offset.
//extern pio_program_offset PIO_Load_Program(pio_device_index /*index*/, const pio_program_t * /*program*/);

// Find a free pio unit.  Try for one with program already loaded.
// Load program if needed.
//
extern pio_handle_t* PIO_Allocate(const pio_program_t *program);

// Initializes a DMA channel for writing 32‑bit words into a PIO TX FIFO.
extern void PIO_DMA_Writer_Init(pio_handle_t* pio_handle);

// Primes a DMA channel with the source buffer and transfer count (does not start DMA).
extern void PIO_DMA_Writer_Prime(dma_channel_index chan, uint32_t* buff, int num_leds);

// Starts one or more previously‑primed DMA channels using a channel bitmask.
extern void PIO_DMA_Writer_Start(void);

#endif // PICOPIO_H

// Endfile: picopio.h