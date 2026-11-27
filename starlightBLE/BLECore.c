// File: BLECore.c

#include "common.h"
#include "BLECore.h"

#include <stdio.h>

#include <btstack.h>

#include <pico/btstack_hci_transport_cyw43.h>
#include <pico/btstack_run_loop_async_context.h>
#include <btstack_event.h>
#include <pico/cyw43_arch.h>
#include <pico/multicore.h>

#include "config.h"
#include "debug_pin.h"
#include "dev_info_service.h"
#include "error.h"
#include "io_service.h"
#include "nus_service.h"

#include "notifier.h"
#include "shell.h"

#define ATT_PACKET 4

#include  <stdio.h>
#include "ble/att_db_util.h"
#include "btstack_util.h"

void list_all_uuids(void)
{
    const uint8_t *db = att_db_util_get_address();
    uint16_t size = att_db_util_get_size();

    printf("ATT DB size: %u bytes\n", size);

    uint16_t offset = 0;

    while (offset < size) {

        // Every attribute entry begins with:
        // [0..1] handle (LE16)
        // [2..3] UUID16 (LE16) or 0 if UUID128
        // [4..5] value_len (LE16)
        // [6..]  value bytes (value_len long)

        uint16_t handle   = little_endian_read_16(db, offset + 0);
        uint16_t uuid16   = little_endian_read_16(db, offset + 2);
        uint16_t vlen     = little_endian_read_16(db, offset + 4);
        const uint8_t *val = db + offset + 6;

        if (uuid16 != 0) {
            // Standard 16-bit UUID
            printf("Handle 0x%04X UUID16 0x%04X\n", handle, uuid16);
        } else {
            // 128-bit UUID stored in value bytes
            printf("Handle 0x%04X UUID128 ", handle);
            for (int i = 0; i < 16; i++) {
                printf("%02X", val[i]);
            }
            printf("\n");
        }

        offset += 6 + vlen;
    }
}



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
        // printf("att_server_packet_handler: packet_type=%u, channel=%u, size=%u\n", packet_type, channel, size);

        uint8_t event = packet[0];
        uint8_t len = packet[1];
        const uint8_t *p = &packet[2];

        switch (event) 
        {
            case ATT_WRITE_REQUEST:
            case ATT_WRITE_COMMAND:
                printf("[ATT] Write to handle 0x%04x [%u]\n", len, *p);
                break;

            case ATT_READ_REQUEST:
                printf("[ATT] Read request for handle 0x%04x\n", len);
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
                // uint16_t mtu = att_server_get_mtu(connection_handle);
                // printf("[ATT] MTU_EXCHANGE_COMPLETE, handle=0x%04x, MTU = %u (payload %u bytes)\n", connection_handle, mtu, mtu - 3);
                break;
            }
            default:
                printf("[ATT] Opcode 0x%02X, handle=0x%04x, [%u]\n", event, len, *p);
                break;
        }
    }
    else { printf("%s: Unknown packet type '%d'!\n", __func__, packet_type); }
}


// -----------------------------------------------------------------------------
// Packet handler (HCI events)
// -----------------------------------------------------------------------------


static void HCI_packet_handler(uint8_t packet_type,
                               uint16_t channel,
                               uint8_t *packet,
                               uint16_t size)
{
//    printf("HCI_packet_handler: packet_type=%u, channel=%u, size=%u\n", packet_type, channel, size);

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

                        printf("[HCI] Connected, handle=0x%04x, MTU = %u (payload %u bytes)\n", connection_handle, mtu, mtu - 3);
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
                        printf("[HCI] unknown sub %d\n", sub);
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
                printf("[HCI] Disconnected < < < * * *\n");
                connection_handle = HCI_CON_HANDLE_INVALID;
                LED_Connected(false);
                break;
            }
            case HCI_EVENT_COMMAND_COMPLETE:
            {
//                printf("[HCI] Command Complete < < < * * *\n");
                // printf("CMD_COMPLETE opcode=0x%04x status=0x%02x\n",
                //     little_endian_read_16(packet, 3),
                //     packet[5]);
                break;
            }
            case 0x60:   // CYW43 vendor-specific event
            case 0x61:   // CYW43 vendor-specific event
            case 0x6E:   // CYW43 vendor-specific event
            case 0xE7:   // CYW43 vendor-specific event
            case 0xFF:   // CYW43 vendor-specific event
                break;

                 printf("[HCI] Vendor-specific event 0x%02X, size=%u\n", event, size);
                break;
            case 0x13:   // L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE
//                 uint8_t subevent = gatt_event_get_subevent_code(packet);
//                 printf("[HCI] L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE\n");
                break;
            default:
            {
                printf("[HCI] unhandled Event 0x%02X, size=%u      * * * * * *\n", event, size);
                break;
            }
        }
    }
    else { printf("%s: packet_type=%u, channel=%u, size=%u\n", __func__, packet_type, channel, size); }
}


//--------------------------------------------------

PRIVATE uint16_t read_callback(hci_con_handle_t con_handle, handle_t attribute_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    UNUSED(con_handle);  // Not used in this callback, but required by the function signature.

    uint16_t bytes_read = 0;
    
    // printf("read_callback: con=0x%04x, att=0x%04x, offset=%u, size=%u\n", con_handle, attribute_handle, offset, buffer_size);

    if (nus_service_handle_match(attribute_handle))
    {
        bytes_read = nus_service_read(attribute_handle, offset, buffer, buffer_size);

        if (!bytes_read) { printf("read_callback: nus_service_read returned false\n"); }
    }
    else if (IO_Service_Handle_Match(attribute_handle))
    {
        bytes_read = IO_Service_Read(attribute_handle, offset, buffer, buffer_size);

        if (!bytes_read) { printf("read_callback: io_service_read returned false\n"); }
    }
    else { printf("read_callback: handle (0x%04x) not handled!\n", attribute_handle); }

    // printf("read_callback: Done.\n\n");

    return bytes_read;
}


PRIVATE int write_callback(hci_con_handle_t con_handle, handle_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(con_handle);  // Not used in this callback, but required by the function signature.
//Debug_Pin_Set(1);
    int result = 0;

    if (attribute_handle)
    {
        //printf("write_callback: con=0x%04x, att=0x%04x, trans=%u, offset=%u, size=%u [", con_handle, attribute_handle, transaction_mode, offset, buffer_size);
        // for (int i=0; i<buffer_size; i++) { printf("%02x ", buffer[i]); } printf("]\n");

        if (nus_service_handle_match(attribute_handle))
        {
            result = nus_service_write(attribute_handle, transaction_mode, offset, buffer, buffer_size);

            if (result) { printf("write_callback: nus_service_write returned code: %d\n", result); }
        }
        else if (IO_Service_Handle_Match(attribute_handle))
        {
            result = IO_Service_Write(attribute_handle, transaction_mode, offset, buffer, buffer_size);

            if (result) { printf("write_callback: io_service_write returned code: %d\n", result); }
        }
        else { printf("write_callback: handle (0x%04x) not handled!\n", attribute_handle); }

        //printf("write_callback: Done.\n\n");
    }
//Debug_Pin_Set(0);

    return result;
}


//-------------------------------------------
#define MY_TASK_PERIOD_MS 10000


static void my_task_handler(btstack_timer_source_t *ts) 
{
    uint32_t now = time_us_32();
//    printf("my_task_handler: Tick %u\n", now);

    if (connection_handle != HCI_CON_HANDLE_INVALID) 
    {
        static char buff[50];
        snprintf(buff, sizeof(buff), "Tick: %lu\n", now);
        Post_Output(buff, strlen(buff));
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

PUBLIC void BLECore_main(void) 
{
    printf("BLECore: starting.\n");

    Notifier_Initialize();

//    cyw43_hal_set_printf(silent_printf);   // suppress CYW43 internal prints

    if (cyw43_arch_init()) 
    {
        printf("BLECore: cyw43_arch_init failed\n");
        return; // -1;
    }

    cyw43_arch_disable_sta_mode();      // “Stop Wi‑Fi Station mode completely.”
    cyw43_arch_disable_ap_mode();       // “Turn off Wi‑Fi Access Point mode completely.”
 
    btstack_memory_init();

    hci_init(hci_transport_cyw43_instance(), NULL);

    l2cap_init();

    sm_init();

    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);    // “My device has no display and no input. I cannot show a passkey, I cannot enter a passkey, and I cannot confirm anything.”
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);      // “I want devices to bond after pairing.”

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

    char error_buffer[ERROR_BUFFER_SIZE];

    Error_Set_Buffer(error_buffer, sizeof(error_buffer));

    if (!Load_Config()) { printf("CANT LOAD CONFIG!\n"); }

    att_server_init(att_db_util_get_address(), read_callback, write_callback);
    att_server_register_packet_handler(att_server_packet_handler);

    gap_advertisements_set_params(0x30, 0x30, 0, 0, 0, 0, 0);
    gap_advertisements_set_data(sizeof(adv_data), adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t*)scan_resp_data);
    gap_advertisements_enable(1);

    hci_power_control(HCI_POWER_ON);

    LED_Startup_Blink(true);

    static btstack_timer_source_t my_task;
    my_task.process = my_task_handler;

    btstack_run_loop_set_timer(&my_task, MY_TASK_PERIOD_MS);
    btstack_run_loop_add_timer(&my_task);

    printf("BLECore: Advertising as '%s'...\n", Node_Name);

    bd_addr_t addr;
    gap_local_bd_addr(addr);
    printf("BLE Address: %s\n", bd_addr_to_str(addr));

// uint8_t mac[6];
// cyw43_wifi_get_mac(&cyw43_state, 0, mac);
// printf("WiFi MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
//        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

//    att_dump_all_handles();   // ************************************ DEBUG *******************************

//void att_dump_attributes_printf(void);
//att_dump_attributes_printf();

//att_dump_attributes();   // ************************************ DEBUG *******************************
  // list_all_uuids();   // ************************************ DEBUG *******************************


    btstack_run_loop_execute();     // Never returns.
    // if (Process_Config(config_buff, true))
    // {
    // }
    // else
    // {
    //     Error_Dump("*** config error");
    //     while (1) { LED_Startup_Blink(true); }
    // }
}


// End of file: BLECore.c


//--------------------------------------------------

// #include "btstack_linked_list.h"
// #include "ble/att_db.h"
// #include "btstack_util.h"

// typedef struct {
//     uint16_t handle;
//     uint16_t flags;
//     uint16_t uuid16;
//     const uint8_t *uuid128;
//     uint16_t value_len;
//     uint8_t *value;
// } att_db_entry_t;

// extern btstack_linked_list_t *att_db_get_list(void);

// void att_dump_all_handles(void) 
// {
//     btstack_linked_list_t *att_db = (const btstack_linked_list_t *)att_db_get_list();

//     btstack_linked_list_iterator_t it;
//     btstack_linked_list_iterator_init(&it, att_db);

//     printf("=== ATT DB Dump ===\n");

//     while (btstack_linked_list_iterator_has_next(&it)) {
//         att_db_entry_t *e = (att_db_entry_t *) btstack_linked_list_iterator_next(&it);

//         printf("Handle 0x%04X  Flags 0x%04X  Len %u  ",
//                e->handle, e->flags, e->value_len);

//         if (e->uuid128) {
//             printf("UUID128 ");
//             for (int i = 0; i < 16; i++) printf("%02X", e->uuid128[i]);
//         } else {
//             printf("UUID16 0x%04X", e->uuid16);
//         }

//         printf("\n");
//     }

//     printf("=== END ===\n");
// }
