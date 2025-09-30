#include "common.h"
#include "btstdio.h"

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include <btstack.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "obled.h"


#define BLUETOOTH_PORTNAME "Starlight"

#define RFCOMM_SERVER_CHANNEL 1

PRIVATE void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

PRIVATE uint16_t rfcomm_channel_id;
PRIVATE uint8_t  spp_service_buffer[150];
PRIVATE btstack_packet_callback_registration_t hci_event_callback_registration;

PRIVATE volatile char* lineBuffer = 0;
PRIVATE volatile uint16_t lineBufferSize = 0;

PRIVATE queue_t BlueTooth_Receive_Queue;

PUBLIC volatile bool BlueTooth_Connected = false;

volatile bool PowerOn_Failed = false;

PRIVATE async_context_t *context = 0;
PRIVATE async_when_pending_worker_t worker;

/* @section SPP Service Setup 
 *s
 * @text To provide an SPP service, the L2CAP, RFCOMM, and SDP protocol layers 
 * are required. After setting up an RFCOMM service with channel nubmer
 * RFCOMM_SERVER_CHANNEL, an SDP record is created and registered with the SDP server.
 * Example code for SPP service setup is
 * provided in Listing SPPSetup. The SDP record created by function
 * spp_create_sdp_record consists of a basic SPP definition that uses the provided
 * RFCOMM channel ID and service name. For more details, please have a look at it
 * in \path{src/sdp_util.c}. 
 * The SDP record is created on the fly in RAM and is deterministic.
 * To preserve valuable RAM, the result could be stored as constant data inside the ROM.   
 */

/* LISTING_START(SPPSetup): SPP service setup */ 
PRIVATE void spp_service_setup(void){

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();

#ifdef ENABLE_BLE
    sm_init();    // Initialize LE Security Manager. Needed for cross-transport key derivation
#endif

    rfcomm_init();
    rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff);  // reserved channel, mtu limited by l2cap

    // init SDP, create record for SPP and register with SDP
    sdp_init();
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    spp_create_sdp_record(spp_service_buffer, 0x10001, RFCOMM_SERVER_CHANNEL, BLUETOOTH_PORTNAME);
    sdp_register_service(spp_service_buffer);
   // PRINTF("SDP service record size: %u\n", de_get_len(spp_service_buffer));
}
/* LISTING_END */

char *PET_to_string(int x)
{
    switch (x)
    {
        case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
        {
            return "BTSTACK_EVENT_NR_CONNECTIONS_CHANGED";
            break;
        }
        case BTSTACK_EVENT_STATE:
        {
            return "BTSTACK_EVENT_STATE";
            break;
        }
        case BTSTACK_EVENT_SCAN_MODE_CHANGED:
        {
            return "BTSTACK_EVENT_SCAN_MODE_CHANGED";
            break;
        }
        case HCI_EVENT_COMMAND_COMPLETE:
        {
            return "HCI_EVENT_COMMAND_COMPLETE";
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE:
        {
            return "HCI_EVENT_DISCONNECTION_COMPLETE";
            break;
        }
        case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
        {
            return "HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS";
            break;
        }
        case HCI_EVENT_TRANSPORT_PACKET_SENT:
        {
            return "HCI_EVENT_TRANSPORT_PACKET_SENT";
            break;
        }
        case HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE:
        {
            return "HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE";
            break;
        }
        case HCI_EVENT_LINK_KEY_REQUEST:
        {
            return "HCI_EVENT_LINK_KEY_REQUEST";
            break;
        }
        case RFCOMM_EVENT_REMOTE_MODEM_STATUS:
        {
            return "RFCOMM_EVENT_REMOTE_MODEM_STATUS";
            break;
        }
        case RFCOMM_EVENT_PORT_CONFIGURATION:
        {
            return "RFCOMM_EVENT_PORT_CONFIGURATION";
            break;
        }
    }
    return "<<<UNMAPPED>>";
}



/* @section Bluetooth Logic 
 * @text The Bluetooth logic is implemented within the 
 * packet handler, see Listing SppServerPacketHandler. In this example, 
 * the following events are passed sequentially: 
 * - BTSTACK_EVENT_STATE,
 * - HCI_EVENT_PIN_CODE_REQUEST (Standard pairing) or 
 * - HCI_EVENT_USER_CONFIRMATION_REQUEST (Secure Simple Pairing),
 * - RFCOMM_EVENT_INCOMING_CONNECTION,
 * - RFCOMM_EVENT_CHANNEL_OPENED, 
* - RFCOMM_EVETN_CAN_SEND_NOW, and
 * - RFCOMM_EVENT_CHANNEL_CLOSED
 */

/* @text Upon receiving HCI_EVENT_PIN_CODE_REQUEST event, we need to handle
 * authentication. Here, we use a fixed PIN code "0000".
 *
 * When HCI_EVENT_USER_CONFIRMATION_REQUEST is received, the user will be 
 * asked to accept the pairing request. If the IO capability is set to 
 * SSP_IO_CAPABILITY_DISPLAY_YES_NO, the request will be automatically accepted.
 *
 * The RFCOMM_EVENT_INCOMING_CONNECTION event indicates an incoming connection.
 * Here, the connection is accepted. More logic is need, if you want to handle connections
 * from multiple clients. The incoming RFCOMM connection event contains the RFCOMM
 * channel number used during the SPP setup phase and the newly assigned RFCOMM
 * channel ID that is used by all BTstack commands and events.
 *
 * If RFCOMM_EVENT_CHANNEL_OPENED event returns status greater then 0,
 * then the channel establishment has failed (rare case, e.g., client crashes).
 * On successful connection, the RFCOMM channel ID and MTU for this
 * channel are made available to the heartbeat counter. After opening the RFCOMM channel, 
 * the communication between client and the application
 * takes place. In this example, the timer handler increases the real counter every
 * second. 
 *
 * RFCOMM_EVENT_CAN_SEND_NOW indicates that it's possible to send an RFCOMM packet
 * on the rfcomm_cid that is include
 */ 

PRIVATE void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);

    bd_addr_t event_addr;
    uint8_t   rfcomm_channel_nr;
    uint16_t  mtu;
    int i;

    switch (packet_type) 
    {
        case HCI_EVENT_PACKET:
        {
            uint8_t pet = hci_event_packet_get_type(packet);

            // D(DEBUG_BLUETOOTH, PRINTF("HCI_EVENT: pet = %X\n", pet);)

            switch (pet) 
            {
                case HCI_EVENT_PIN_CODE_REQUEST:
                {
                    // inform about pin code request
                    PRINTF("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
                    break;
                }
                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                {
                    // ssp: inform about user confirmation request
                    PRINTF("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", little_endian_read_32(packet, 8));
                    PRINTF("SSP User Confirmation Auto accept\n");
                    break;
                }
                case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
                {
                    PRINTF("PAIRING_COMPLETE\n");
// ObLED_On();
// sleep_ms(500);
// ObLED_Off();
// sleep_ms(500);
// ObLED_On();
// sleep_ms(500);
// ObLED_Off();

                    break;
                }
                case RFCOMM_EVENT_INCOMING_CONNECTION:
                {
                    D(DEBUG_BLUETOOTH, PRINTF("RFCOMM_EVENT_INCOMING_CONNECTION\n");)
                    rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr);
                    rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
                    rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
                    D(DEBUG_BLUETOOTH, PRINTF("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));)
                    rfcomm_accept_connection(rfcomm_channel_id);
                    break;
                }
                case RFCOMM_EVENT_CHANNEL_OPENED:
                {
                    if (rfcomm_event_channel_opened_get_status(packet)) 
                    {
                        PRINTF("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
                    } else 
                    {
                        rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                        mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                        D(DEBUG_BLUETOOTH, PRINTF("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);)
                        BlueTooth_Connected = true;
                        ObLED_On();
                    }
                    break;
                }
                case RFCOMM_EVENT_CAN_SEND_NOW:
                {
                    if (lineBufferSize > 0 && lineBuffer)
                    {
//                        PRINTF("RFCOMM can send now: size %d\n", lineBufferSize);
//                        rfcomm_send(rfcomm_channel_id, (uint8_t*)lineBuffer, (uint16_t)strlen((const char*)lineBuffer));  
                        rfcomm_send(rfcomm_channel_id, (uint8_t*)lineBuffer, lineBufferSize);  
//                        PRINTF("RFCOMM did send: size %d\n", lineBufferSize);
                    }
                    lineBuffer = 0;   lineBufferSize = 0;
                    break;
                }
                case RFCOMM_EVENT_CHANNEL_CLOSED:
                {
                    PRINTF("RFCOMM channel closed\n");
                    rfcomm_channel_id = 0;
                    BlueTooth_Connected = false;
                    ObLED_Off();
                    break;
                }
                case BTSTACK_EVENT_POWERON_FAILED:
                {
                    PowerOn_Failed = true;
                    break;
                }
				case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:					// (0x61)
				case BTSTACK_EVENT_SCAN_MODE_CHANGED:						// (0x66)
				case BTSTACK_EVENT_STATE:									// (0x60)

				case GAP_EVENT_SECURITY_LEVEL:								// (0xD8)

                case HCI_EVENT_COMMAND_COMPLETE:							// (0x0E)
				case HCI_EVENT_COMMAND_STATUS:								// (0x0F)
				case HCI_EVENT_CONNECTION_COMPLETE:							// (0x03)
				case HCI_EVENT_CONNECTION_REQUEST:							// (0x04)
				case HCI_EVENT_DISCONNECTION_COMPLETE:						// (0x05)
				case HCI_EVENT_ENCRYPTION_CHANGE:							// (0x08)
				case HCI_EVENT_LINK_KEY_REQUEST:							// (0x17)
				case HCI_EVENT_MAX_SLOTS_CHANGED:							// (0x1B)
				case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:					// (0x13)
				case HCI_EVENT_READ_REMOTE_EXTENDED_FEATURES_COMPLETE:		// (0x23)
				case HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE: 	// (0x0B)              
				case HCI_EVENT_TRANSPORT_PACKET_SENT:						// (0x6E)

                case RFCOMM_EVENT_PORT_CONFIGURATION:						// (0x88)
				case RFCOMM_EVENT_REMOTE_MODEM_STATUS:						// (0x87)
                {
//                    D(DEBUG_BLUETOOTH, PRINTF("BT[0x%X]-%s\n", pet, PET_to_string(pet));)
                    break;
                }
                default:
                {
                    PRINTF("(Bluetooth) packet_handler: Unknown packet (PET) type 0x%X\n", pet);
                    break;
                }
            }
            break;
        }
        case RFCOMM_DATA_PACKET:
        {
//            D(DEBUG_BLUETOOTH, PRINTF("RFCOMM_DATA_PACKET: size %d\n", size);)
            ObLED_Off();
            for (i = 0; i < size; i++)
            {
                queue_add_blocking(&BlueTooth_Receive_Queue, &packet[i]);
            }
            ObLED_On();
            break;
        }
        default:
        {
            PRINTF("(Bluetooth) packet_handler: Unknown packet type 0x%X\n", packet_type);
            break;
        }
    }
}

PRIVATE void worker_function(async_context_t *context, struct async_when_pending_worker *worker)
{
    rfcomm_request_can_send_now_event(rfcomm_channel_id);
}

PRIVATE void worker_setup(void)
{
    context = cyw43_arch_async_context();
    worker.do_work = worker_function;
    async_context_add_when_pending_worker(context, &worker);
}

PRIVATE void worker_trigger(void)
{
    async_context_set_work_pending(context, &worker);
}


PUBLIC void BlueTooth_Send_Buffer(uint8_t* buff, size_t n)
{
    if (n)   // Is there something to send?
    {
        if (!lineBufferSize)    // Are we already sending?
        {
            lineBuffer = buff;
            lineBufferSize = n;

            worker_trigger();
        }
        else { printf("BlueTooth_Send_Buffer: Already sending!\n"); }

        while (BlueTooth_Connected && lineBufferSize) { continue; }      // Wait for it to finish sending.
    }
}


// ----------------------------------------------------------------------------------------

PUBLIC void BlueTooth_Server(void)   // This is the main for the second core.
{
    btstack_memory_init();

    queue_init(&BlueTooth_Receive_Queue, sizeof(uint8_t), QUEUE_SIZE);

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) { PRINTF("Bluetooth init failed\n"); return; }

    worker_setup();

    spp_service_setup();

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name(BLUETOOTH_PORTNAME" 00:00:00:00:00:00");

     hci_power_control(HCI_POWER_ON);    // turn on!

    if (PowerOn_Failed)
    {
        printf("GOT HERE: PowerOn_Failed %d\n", PowerOn_Failed);
    }
    else
    {
        btstack_run_loop_execute();
        printf("GOT HERE: btstack_run_loop_execute returned!\n");
    }
}


PUBLIC bool BlueTooth_TryGetPeek(uint8_t* val)
{
    return queue_try_peek(&BlueTooth_Receive_Queue, val);
}

PUBLIC int BlueTooth_TryGetChar()
{
    char val;
    return queue_try_remove(&BlueTooth_Receive_Queue, &val) ? val : PICO_ERROR_TIMEOUT;
}


PUBLIC char BlueTooth_GetChar()
{
    char val;
    queue_remove_blocking(&BlueTooth_Receive_Queue, &val);
    return val;
}


PUBLIC bool BlueTooth_Check_Receive(void)
{
    if (BlueTooth_Connected)
    {
        int val;
        return queue_try_peek(&BlueTooth_Receive_Queue, &val);
    }
    return false;
}


#define MAX_BT_PRINTF_BUFF 1000

#include <stdarg.h>
PUBLIC void BlueTooth_Printf(const char *fmt, ...) 
{
    static uint8_t buff[MAX_BT_PRINTF_BUFF] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);    //vsnprintf BlueTooth_Send_String
    va_end(args);
    // D(DEBUG_BLUETOOTH, printf(buff);)
    // D(DEBUG_BLUETOOTH, fflush(stdout);)
    BlueTooth_Send_String(buff);
}

// PUBLIC void BlueTooth_Printf_faster(const char *fmt, ...) 
// {
//     uint8_t buff[MAX_BT_PRINTF_BUFF] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buff, sizeof(buff), fmt, args);    //vsnprintf BlueTooth_Send_String
//     va_end(args);
//     BlueTooth_Send_String(buff);
// }


// EndFile: bluetooth_stdio.c
