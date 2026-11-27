// File: nus_service.c

#include "common.h"
#include "nus_service.h"

#include <stdio.h>
#include <ble/att_server.h>
#include <ble/att_db_util.h>

#include "notifier.h"
#include "shell.h"
#include "spsc_queue.h"


// -----------------------------------------------------------------------------
// Nordic UART Service UUIDs (128-bit)
// -----------------------------------------------------------------------------
#define UUID_HEAD 0x6E, 0x40
#define UUID_TAIL 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E

PRIVATE const uint8_t      nus_service_uuid[16] = { UUID_HEAD, 0x00, 0x01, UUID_TAIL };
PRIVATE const uint8_t           nus_rx_uuid[16] = { 0x6E, 0x40, 0x00, 0x02, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E };
PRIVATE const uint8_t           nus_tx_uuid[16] = { 0x6E, 0x40, 0x00, 0x03, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E };
PRIVATE const uint8_t nus_event_stream_uuid[16] = { 0x6E, 0x40, 0x00, 0x05, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E };

PRIVATE volatile uint16_t tx_cccd = 0;
PRIVATE volatile uint16_t event_stream_cccd = 0;

// -----------------------------------------------------------------------------
// Buffers and handles
// -----------------------------------------------------------------------------

PRIVATE uint16_t nus_rx_value_handle = 0;
PRIVATE uint16_t nus_tx_value_handle = 0;
PRIVATE uint16_t nus_event_stream_handle = 0;


PUBLIC void Post_Output(const char* buffer, uint16_t buffer_size)
{
    if (buffer && !buffer_size) { buffer_size = strlen(buffer) - 1; /* Don't send NULL */ }

    DBG(0, DEBUG_RESULTS, ">: '%.*s'\n", buffer_size, buffer);

    if (connection_handle != HCI_CON_HANDLE_INVALID)    // Echo back via TX notify
    {
        Notifier_Post_buffer(nus_tx_value_handle, buffer, buffer_size);
    }
}


PUBLIC void Post_Event(const char* buffer, uint16_t buffer_size)
{
    if (*buffer != '\n') { DBG(0, DEBUG_RESULTS, ">: '%.*s'\n", buffer_size, buffer); }

    if (connection_handle != HCI_CON_HANDLE_INVALID)    // Echo back via TX notify
    {
        Notifier_Post_buffer(nus_event_stream_handle, buffer, buffer_size);
    }
}


PUBLIC void nus_service_initialize()
{
    att_db_util_add_service_uuid128(nus_service_uuid);    // Add NUS service

    nus_rx_value_handle = att_db_util_add_characteristic_uuid128(nus_rx_uuid,
        ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE | ATT_PROPERTY_DYNAMIC,
        ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);

    nus_tx_value_handle = att_db_util_add_characteristic_uuid128(nus_tx_uuid,
        ATT_PROPERTY_NOTIFY | ATT_PROPERTY_DYNAMIC,
        ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);

    nus_event_stream_handle = att_db_util_add_characteristic_uuid128(nus_event_stream_uuid,
        ATT_PROPERTY_NOTIFY | ATT_PROPERTY_DYNAMIC,
        ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);
}


PUBLIC bool nus_service_handle_match(uint16_t handle)
{
    return (handle == nus_rx_value_handle 
        ||  handle == nus_tx_value_handle 
        || (handle == nus_tx_value_handle + 1)     // extra for notification enable.
        ||  handle == nus_event_stream_handle 
        || (handle == nus_event_stream_handle + 1)    // extra for notification enable.
    );
}


PUBLIC uint16_t nus_service_read(handle_t handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    UNUSED(handle);  // Not used in this callback, but required by the function signature.
    UNUSED(offset);

    DBG(0, DEBUG_BLESERVICE, "TX (%u bytes)\n", buffer_size);

    if (buffer && buffer_size >= 2)
    {
        *buffer = 0;  *(buffer + 1) = tx_cccd ? 1 : 0;
    }

    return 2;   // Always returns 2.
}


PUBLIC int nus_service_write(handle_t handle, uint16_t transaction_mode, uint16_t offset, uint8_t* buffer, uint16_t buffer_size)
{
    UNUSED(transaction_mode);  // Not used in this callback, but required by the function signature.
    UNUSED(offset);

    int result = ATT_ERROR_UNLIKELY_ERROR;

    if (handle == nus_rx_value_handle) 
    {
        DBG(0, DEBUG_BLESERVICE, "RX (%u bytes): %.*s\n", buffer_size, buffer_size, buffer);
        ble_rx((char*)buffer, buffer_size);
        result = ATT_ERROR_SUCCESS;
    }
    else if (handle == nus_tx_value_handle + 1)     // notification enable flag.
    {
        tx_cccd = *buffer;
        result = ATT_ERROR_SUCCESS;
    }
    else if (handle == nus_event_stream_handle + 1)     // notification enable flag.
    {
        event_stream_cccd = *buffer;
        result = ATT_ERROR_SUCCESS;
    }
    else 
    {
        DBG(0, DEBUG_BLESERVICE, "Write to unknown handle: 0x%04x\n", handle); 
        result = ATT_ERROR_ATTRIBUTE_NOT_FOUND;
    }

    return result;
}


// End of file: nus_service.c
