// File: BLECore.c

#include "common.h"
#include "BLECore.h"

#include <stdio.h>

#include <btstack.h>
#include <btstack_event.h>
#include <btstack_util.h>

#include <ble/att_db_util.h>

#include <pico/btstack_hci_transport_cyw43.h>
#include <pico/btstack_run_loop_async_context.h>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>

#include "config.h"
//#include "debug_pin.h"
#include "dev_info_service.h"
#include "error.h"
#include "io_service.h"
#include "notifier.h"
#include "nus_service.h"
#include "shell.h"
#include "variables.h"
#include "watcher.h"


#define MY_TASK_PERIOD_MS 10000

PUBLIC volatile uint32_t Tick_Count = 0;     // Incremented every tick interval (50ms).

extern variable_t* Tick_Var;     // Pointer to the TICK variable (for watching).

// -----------------------------------------------------------------------------
// Buffers and handles
// -----------------------------------------------------------------------------

PUBLIC volatile hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;

// -----------------------------------------------------------------------------
// Advertising data
// -----------------------------------------------------------------------------

PRIVATE const uint8_t adv_data[] = {
    0x02, 0x01, 0x06, // Flags
    0x11, 0x07,       // Incomplete 128-bit UUID list
    0x48, 0x43, 0x45, 0x54, 0x54, 0x49, 0x42, 0x45,      // Only show ABLE service.
    0x4C, 0x42, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

#define ADV_COMPLETE_LOCAL_NAME 0x09
#define ADV_MANUFACTURER_SPECIFIC_DATA 0xFF

PRIVATE char Node_Name[MAX_NODE_NAME_SIZE + 1];
PRIVATE char scan_resp_data[MAX_SCAN_RESP_DATA_SIZE];


PUBLIC void BLE_Set_Node_Name(char* name)
{
    _Static_assert(MAX_SCAN_RESP_DATA_SIZE >= MAX_NODE_NAME_SIZE + 8, "scan_resp_data too small for node name");

    if (name && *name)
    {
        char* p = scan_resp_data;
        size_t len = strlen(name);

        if (len > MAX_NODE_NAME_SIZE) { len = MAX_NODE_NAME_SIZE; }

        *p++ = len + 1;
        *p++ = ADV_COMPLETE_LOCAL_NAME;

        strncpy(p, name, len + 1);
        strncpy(Node_Name, name, len);
        Node_Name[len] = '\0';

        p += len;

        *p++ = 5;
        *p++ = ADV_MANUFACTURER_SPECIFIC_DATA;
        *p++ = 0xFF;   *p++ = 0xFF;
        *p++ = 0x12;   *p++ = 0x34;
    }
}


PRIVATE void LED_Connected(bool on)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (on ? 1 : 0)); // LED on
}


PRIVATE void LED_Startup_Blink(bool fast)
{
    int count = fast ? 4 : 2;
    int delay = fast ? 80 : 250;

    for (int i = 0; i < count; i++)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);  sleep_ms(delay);  // LED on
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);  sleep_ms(delay);  // LED off
    }
}


PRIVATE void att_server_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) 
{
    UNUSED(channel); 
    UNUSED(size);

    if (packet_type == HCI_EVENT_PACKET) 
    {
        DBG(5, DEBUG_BLUETOOTH, " type=%u, size=%u\n", packet_type, size);
//        DBG(0, DEBUG_BLUETOOTH, " type=%u, channel=%u, size=%u\n", packet_type, channel, size);

        uint8_t event = packet[0];
        uint8_t len = packet[1];
        const uint8_t *p = &packet[2];

        switch (event) 
        {
            case ATT_WRITE_REQUEST:
            case ATT_WRITE_COMMAND:
                DBG(0, DEBUG_BLUETOOTH, "[ATT] Write to handle 0x%04x [%u]\n", len, *p);
                break;

            case ATT_READ_REQUEST:
                DBG(0, DEBUG_BLUETOOTH, "[ATT] Read request for handle 0x%04x\n", len);
                break;

            case ATT_EVENT_CAN_SEND_NOW:
                Notifier_Process_Can_Send_Now_Event();
                break;

            case HCI_EVENT_LE_META:
            case ATT_EVENT_CONNECTED:
            case HCI_EVENT_CONNECTION_COMPLETE:
            case ATT_EVENT_DISCONNECTED:
            case HCI_EVENT_DISCONNECTION_COMPLETE:
                break;

            case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            {
                uint16_t mtu = att_server_get_mtu(connection_handle);
                DBG(0, DEBUG_BLUETOOTH, "[ATT] MTU_EXCHANGE_COMPLETE, handle=0x%04x, MTU = %u (payload %u bytes)\n", connection_handle, mtu, mtu - 3);
                break;
            }
            default:
                DBG(0, DEBUG_BLUETOOTH, "[ATT] Opcode 0x%02X, handle=0x%04x, [%u]\n", event, len, *p);
                break;
        }
    }
    else { DBG(0, DEBUG_BLUETOOTH, "Unknown packet type '%d'!\n", packet_type); }
}


// -----------------------------------------------------------------------------
// Packet handler (HCI events)
// -----------------------------------------------------------------------------


static void HCI_packet_handler(uint8_t packet_type,
                               uint16_t channel,
                               uint8_t *packet,
                               uint16_t size)
{
    DBG(5, DEBUG_BLUETOOTH, " type=%u, size=%u\n", packet_type, size);
//    DBG(0, DEBUG_BLUETOOTH, " type=%u, channel=%u, size=%u\n", packet_type, channel, size);

    // -------------------------------
    // HCI EVENTS (packet_type == 0x04)
    // -------------------------------
    if (packet_type == HCI_EVENT_PACKET) 
    {
        uint8_t event = hci_event_packet_get_type(packet);

        switch (event) 
        {
            // ---------------------------
            // LE Connection Complete
            // ---------------------------
            case HCI_EVENT_LE_META: 
            {
                uint8_t sub = hci_event_le_meta_get_subevent_code(packet);

                switch (sub)
                {
                    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    {
                        connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                        uint16_t mtu = att_server_get_mtu(connection_handle);

                        LED_Connected(true);

                        DBG(0, DEBUG_BLUETOOTH, "[HCI] Connected, handle=0x%04x, MTU = %u (payload %u bytes)\n", connection_handle, mtu, mtu - 3);
                        break;
                    }
                    case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    case HCI_SUBEVENT_LE_REMOTE_CONNECTION_PARAMETER_REQUEST:
                    case HCI_SUBEVENT_LE_CHANNEL_SELECTION_ALGORITHM:
                    case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                    {
                        break;
                    }
                    default:
                    {
                        DBG(0, DEBUG_BLUETOOTH, "[HCI] unknown sub %d\n", sub);
                        break;
                    }
                }
                break;
            }

            // ---------------------------
            // Disconnection
            // ---------------------------
            case HCI_EVENT_DISCONNECTION_COMPLETE:
            {
                DBG(0, DEBUG_BLUETOOTH, "[HCI] Disconnected < < < * * *\n");
                connection_handle = HCI_CON_HANDLE_INVALID;
                Notifier_Reset();
                LED_Connected(false);
                break;
            }
            case HCI_EVENT_COMMAND_COMPLETE:
            {
                DBG(0, DEBUG_BLUETOOTH, "[HCI] Command Complete < < < * * *\n");
                break;
            }
            case 0x60:   // CYW43 vendor-specific event
            case 0x61:   // CYW43 vendor-specific event
            case 0x6E:   // CYW43 vendor-specific event
            case 0xE7:   // CYW43 vendor-specific event
            case 0xFF:   // CYW43 vendor-specific event
                break;

//                DBG(0, DEBUG_BLUETOOTH, "[HCI] Vendor-specific event 0x%02X, size=%u\n", event, size);
//                break;
            case 0x13:   // L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE
//                 uint8_t subevent = gatt_event_get_subevent_code(packet);
//                 DBG(0, DEBUG_BLUETOOTH, "[HCI] L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE\n");
                break;
            default:
            {
                DBG(0, DEBUG_BLUETOOTH, "[HCI] unhandled Event 0x%02X, size=%u      * * * * * *\n", event, size);
                break;
            }
        }
    }
    else { DBG(0, DEBUG_BLUETOOTH, "packet_type=%u, channel=%u, size=%u\n", packet_type, channel, size); }
}


//--------------------------------------------------

PRIVATE uint16_t read_callback(hci_con_handle_t con_handle, handle_t attr_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    UNUSED(con_handle);  // Not used in this callback, but required by the function signature.

    uint16_t bytes_read = 0;
    
    DBG(0, DEBUG_BLUETOOTH, "con=0x%04x, att=0x%04x, offset=%u, size=%u\n", con_handle, attr_handle, offset, buffer_size);

    if (nus_service_handle_match(attr_handle))
    {
        bytes_read = nus_service_read(attr_handle, offset, buffer, buffer_size);

        if (!bytes_read) { DBG(0, DEBUG_BLUETOOTH, "nus_service_read returned false\n"); }
    }
    else if (IO_Service_Handle_Match(attr_handle))
    {
        bytes_read = IO_Service_Read(attr_handle, offset, buffer, buffer_size);

        if (!bytes_read) { DBG(0, DEBUG_BLUETOOTH, "io_service_read returned false\n"); }
    }
    else { DBG(0, DEBUG_BLUETOOTH, "handle (0x%04x) not handled!\n", attr_handle); }

    DBG(0, DEBUG_BLUETOOTH, "Done.\n\n");

    return bytes_read;
}


PRIVATE int write_callback(hci_con_handle_t con_handle, handle_t attr_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(con_handle);  // Not used in this callback, but required by the function signature.
//Debug_Pin_Set(1);
    int result = 0;

    if (attr_handle)
    {
        DBG(0, DEBUG_BLUETOOTH, "con=0x%04x, att=0x%04x, trans=%u, offset=%u, size=%u [", con_handle, attr_handle, transaction_mode, offset, buffer_size);
        // for (int i=0; i<buffer_size; i++) { printf("%02x ", buffer[i]); } printf("]\n");

        if (nus_service_handle_match(attr_handle))
        {
            result = nus_service_write(attr_handle, transaction_mode, offset, buffer, buffer_size);

            if (result) { DBG(0, DEBUG_BLUETOOTH, "nus_service_write returned code: %d\n", result); }
        }
        else if (IO_Service_Handle_Match(attr_handle))
        {
            result = IO_Service_Write(attr_handle, transaction_mode, offset, buffer, buffer_size);

            if (result) { DBG(0, DEBUG_BLUETOOTH, "io_service_write returned code: %d\n", result); }
        }
        else { DBG(0, DEBUG_BLUETOOTH, "handle (0x%04x) not handled!\n", attr_handle); }

        DBG(0, DEBUG_BLUETOOTH, "write_callback: Done.\n\n");
    }
//Debug_Pin_Set(0);

    return result;
}


//-------------------------------------------

static void my_task_handler(btstack_timer_source_t *ts) 
{
    if (connection_handle != HCI_CON_HANDLE_INVALID) 
    {
        Tick_Count++;

        if (Tick_Var) { Var_Notify(Tick_Var); }  // Notify any watchers of the TICK variable.
    }

    // your periodic work
    btstack_run_loop_set_timer(ts, MY_TASK_PERIOD_MS); // 100 ms
    btstack_run_loop_add_timer(ts);
}


// extern void cyw43_hal_set_printf(void (*printf_func)(const char *fmt, ...));

// static void silent_printf(const char *fmt, ...) {
//     // do nothing
// }


/*
// PRIVATE const uint8_t* config_buff = "\
// ABLE 454 28487\n\
// ADV ABLEBIT111\n\
// PIN IN   12   IN1    GPIO input pin\n\
// PIN BUS 15 14 13  BUS1    GPIO output pin set.\n\
// PIN STRIP 10 5 0 newstrip A fancy digital led strip\n\
// ";

//PIN LED 13 12 11   LED1    Single LED RGB.\n\
//PIN BUS 13 12 11  BUS1    GPIO output pin set.\n\
//PIN OUT  12   OUT1   GPIO output pin\n\
//PIN PWM  13   PWM1   PWM output pin\n\
// PIN PWM  15   PWM1   PWM output pin\n\
// PIN IN   16   IN1    GPIO input pin\n\
// PIN OUT  17   OUT1   GPIO output pin\n\
// PIN BUS 18 19 20 21 3 4 5 6  BUS1    GPIO output pin set.\n\
// PIN LED 26            LED2    Single LED Red only.\n\
// PIN LED . 27          LED3    Single LED Green only.\n\
// PIN LED . . 28        LED4    Single LED Blue only.\n\
// PIN LED . . . 29      LED5    Single LED White only.\n\
// PIN STRIP 22 RGB 300  STRIP1  LED Strip with 300 RGB leds.\n\
// PIN UART 23 19200 8N1 UART1   Uart service.\n\
*/

PUBLIC const char* BLE_Address_String = "";

PUBLIC void get_ble_addr(void)
{
    bd_addr_t addr;
    gap_local_bd_addr(addr);
    BLE_Address_String = bd_addr_to_str(addr);

    DBG(0, DEBUG_BLUETOOTH, "%s\n", BLE_Address_String);
}


PUBLIC void BLECore_main(void) 
{
    Debug_Mask = DEBUG_MASK_STARTUP;
    Debug_Level = DEBUG_LEVEL_STARTUP;
    
    DBG(0, DEBUG_BLUETOOTH, "Starting.\n");

    Notifier_Initialize();

//    cyw43_hal_set_printf(silent_printf);   // suppress CYW43 internal prints

    if (cyw43_arch_init()) 
    {
        DBG(0, DEBUG_BLUETOOTH, "cyw43_arch_init failed\n");
        return; // -1;
    }

    cyw43_arch_disable_sta_mode();      // Stop Wi-Fi Station mode completely.
    cyw43_arch_disable_ap_mode();       // Turn off Wi-Fi Access Point mode completely.
 
    btstack_memory_init();

    hci_init(hci_transport_cyw43_instance(), NULL);

    l2cap_init();

    sm_init();

    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);    // My device has no display and no input. I cannot show a passkey, I cannot enter a passkey, and I cannot confirm anything.
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);      // I want devices to bond after pairing.

    LED_Startup_Blink(false);

    static btstack_packet_callback_registration_t hci_cb;
    hci_cb.callback = &HCI_packet_handler;
    hci_add_event_handler(&hci_cb);

    BLE_Set_Node_Name("ABLEBIT111");

    // Build GATT table.
    //
    att_db_util_init();

    di_service_initialize();
    nus_service_initialize();
    IO_Service_Initialize();

    static char error_buffer[ERROR_BUFFER_SIZE];
    static char watcher_buffer[WATCHER_BUFFER_SIZE];

    Error_Set_Buffer(error_buffer, sizeof(error_buffer));
    Watcher_Set_Buffer(watcher_buffer, sizeof(watcher_buffer));

    if (!Load_Config()) { DBG(0, DEBUG_BLUETOOTH, "Can't load CONFIG!\n"); }     // ----------------------------------------- Needs to be updated to new environment.

    att_server_init(att_db_util_get_address(), read_callback, write_callback);
    att_server_register_packet_handler(att_server_packet_handler);

    gap_advertisements_set_params(0x30, 0x30, 0, 0, 0, 0, 0);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t*)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t*)scan_resp_data);
    gap_advertisements_enable(1);

    hci_power_control(HCI_POWER_ON);

    LED_Startup_Blink(true);

    static btstack_timer_source_t my_task;
    my_task.process = my_task_handler;

    btstack_run_loop_set_timer(&my_task, MY_TASK_PERIOD_MS);
    btstack_run_loop_add_timer(&my_task);

    DBG(0, DEBUG_BLUETOOTH, "Advertising as '%s'...\n", Node_Name);

    get_ble_addr();

//    list_all_uuids();   // ************************************ DEBUG *******************************

    Debug_Mask = DEBUG_MASK_RUNNING;
    Debug_Level = DEBUG_LEVEL_RUNNING;

    btstack_run_loop_execute();     // Never returns.
}

//--------------------------------------------------

PUBLIC void list_all_uuids(void)
{
    const uint8_t *db = att_db_util_get_address();
    uint16_t total = att_db_util_get_size();

    DBG(0, DEBUG_BLUETOOTH, "Size: %u bytes, version %u\n", total, db[0]);

    uint16_t offset = 1;                        // Skip the 1-byte DB version.

    while ((uint32_t)offset + 6 <= total)
    {
        uint16_t entry_size = little_endian_read_16(db, offset + 0);

        if (entry_size == 0) { break; }         // End marker.

        if (entry_size < 6 || (uint32_t)offset + entry_size > total)
        {
            DBG(0, DEBUG_BLUETOOTH, "Bad entry size %u at offset %u\n", entry_size, offset);
            break;
        }

        uint16_t flags  = little_endian_read_16(db, offset + 2);
        uint16_t handle = little_endian_read_16(db, offset + 4);
        const uint8_t *uuid = db + offset + 6;

        if (flags & ATT_PROPERTY_UUID128)
        {
            if (entry_size < 6 + 16) { DBG(0, DEBUG_BLUETOOTH, "Short UUID128 entry at offset %u\n", offset); break; }

            uint8_t uuid_be[16];
            reverse_128(uuid, uuid_be);         // DB stores UUIDs little-endian.
            DBG(0, DEBUG_BLUETOOTH, "Handle 0x%04X flags 0x%04X UUID128 %s\n", handle, flags, uuid128_to_str(uuid_be));
        }
        else
        {
            if (entry_size < 6 + 2) { DBG(0, DEBUG_BLUETOOTH, "Short UUID16 entry at offset %u\n", offset); break; }

            DBG(0, DEBUG_BLUETOOTH, "Handle 0x%04X flags 0x%04X UUID16  0x%04X\n", handle, flags, little_endian_read_16(uuid, 0));
        }

        offset += entry_size;
    }
}


// End of file: BLECore.c
