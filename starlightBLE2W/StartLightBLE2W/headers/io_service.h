// File: io_service.h

#ifndef IO_SERVICE_H
#define IO_SERVICE_H

#include "BLECore.h"
#include "gatt.h"

#define BUILD_VAL(value)  (value)[0]
#define BUILD_2_VAL(value) ((value)[0] + ((value)[1] << 8))
#define BUILD_4_VAL(value) ((value)[0] + ((value)[1] << 8) + ((value)[2] << 16) + ((value)[3] << 24))


typedef enum
{
    IOTYPE_NONE,        // 0

    IOTYPE_INPUT,       // 1
    IOTYPE_OUTPUT,      // 2
    IOTYPE_PWM,         // 3
    IOTYPE_OUTBUS,      // 4
    IOTYPE_LED,         // 5
    IOTYPE_STRIP,       // 6
    
    IOTYPE_MAX          // 7

} IOTYPE;

    // IOTYPE_UART_TX,   // PIO
    // IOTYPE_UART_RX,   // PIO
    // IOTYPE_SPI_TX,    // PIO
    // IOTYPE_SPI_RX,    // PIO
    // IOTYPE_I2C,       // PIO
    // IOTYPE_CAN,       // PIO
    // IOTYPE_TICK,      // Periodic timer tick.
    // IOTYPE_VAR,       // Named and typed variable.
    // IOTYPE_ADC,       // Not available on Pico W.
    // IOTYPE_DAC,       // Not available on any Pico.


// Represents a single ATT (Attribute) entry in the dynamic I/O service table.
// Each ATT_VALUE corresponds to one GATT characteristic that maps to a device
// handler (GPIO, PWM, BUS, LED). The structure stores the characteristic handle,
// the device type, the hardware binding, and optional value storage.
//
typedef struct att_value_t
{
    uint16_t handle;          // GATT attribute handle assigned by BTstack.
                              // Used to match incoming ATT read/write requests.

    IOTYPE   type;            // Device type (INPUT, OUTPUT, PWM, BUS, LED, IOTYPE_STRIP, IOTYPE_OUTBUS).
                              // Determines which ioserv_function_t dispatches
                              // read/write operations for this characteristic.

    uintptr_t iohandle;       // Device-specific hardware binding.
                              // GPIO:     pin number (uint32_t cast to uintprt_t)
                              // PWM:      pin number (uint32_t)
                              // BUS:      pointer to array of pins (uint8_t*)
                              // LED:      packed 32-bit RGBA pin map
                              // IOTYPE_STRIP: 
                              // IOTYPE_OUTBUS: 
                              // NULL means no hardware is associated.

    uint16_t value_len;       // Length of the value buffer (if used).
                              // Most I/O types do not store a value here; they
                              // read/write directly from hardware.

    uintptr_t value;          // Optional backing store for characteristic value.
                              // Typically NULL for dynamic I/O characteristics.
                              // Used only if the characteristic maintains state.

    char name[MAX_WORD_SIZE];
                              // Human-readable name for debugging/logging.
                              // Example: "PIN15", "LED0", "BUS1".
                              // Null-terminated string.

    uint16_t cccd;            // Stores notification status.

    char desc[MAX_DESCRIPTOR_SIZE];

} ATT_VALUE;


#define READ_ONLY true          // new_characteristic is read only.
#define CAN_WRITE false         // new_characteristic can write.

// -----------------------------------------------------------------------------
// Buffers and handles
// -----------------------------------------------------------------------------

extern void IO_Service_Initialize(void);

extern bool IO_Service_Handle_Match(handle_t handle);

extern uint16_t IO_Service_Read(handle_t attribute_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);

extern int IO_Service_Write(handle_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

extern handle_t New_Characteristic(const char* name, bool read_only, gatt_unit_t unit, gatt_format_t format, gatt_exponent_t exponent, IOTYPE type, uint8_t unit_id);

extern void Dump_Srvs_List(char** buffp, size_t* buffsizep);

extern ATT_VALUE* Att_Values_Add(handle_t handle, const char* name, IOTYPE type, uintptr_t iohandle, uint16_t value_len, uintptr_t value, const char* desc);

extern ATT_VALUE* Att_Values_Find_Name(const char* name);

extern const char* Att_Value_Type_String(ATT_VALUE* att_val);

extern void Att_Value_Pin_List(char** buffp, size_t* buffsizep, uint8_t* pin_list);

extern uint16_t Att_Apply_Read(handle_t handle, uint8_t* buffer, uint16_t buffer_size);

extern bool Att_Apply_Write(handle_t handle, const uint8_t* buffer, uint16_t buffer_size);


typedef struct ioserv_function
{
    const char* name;
    bool     (*init) (const char* name, uint8_t* port_pins, size_t pin_count, const char* desc);
    uint16_t (*read) (ATT_VALUE* att_value, uint8_t* value, uint16_t value_len);
    bool     (*write)(ATT_VALUE* att_value, uint32_t val);

} ioserv_function_t;

extern const ioserv_function_t* io_service_get_device(IOTYPE type);


#endif // IO_SERVICE_H


// End of file: io_service.h
