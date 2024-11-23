/*
 * Heavily modified and in no way backward compatible! - Steven A. McClain (2024)
 *
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 */

#include "Common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/util/queue.h>
#include <pico/cyw43_arch.h>

#include <btstack.h>

#include "bluetooth_stdio.h"

#define BLUETOOTH_PORTNAME "Starlight"

#define RFCOMM_SERVER_CHANNEL 1
#define HEARTBEAT_PERIOD_MS 1000

PRIVATE void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

PRIVATE uint16_t rfcomm_channel_id;
PRIVATE uint8_t  spp_service_buffer[150];
PRIVATE btstack_packet_callback_registration_t hci_event_callback_registration;

PRIVATE volatile char* lineBuffer = 0;

PRIVATE queue_t BlueTooth_Receive_Queue;


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
   // printf("SDP service record size: %u\n", de_get_len(spp_service_buffer));
}
/* LISTING_END */

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

            // printf("HCI_EVENT: pet = %X\n", pet);

            switch (pet) 
            {
                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    printf("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // ssp: inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\n");
                    break;

                case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
                {
                    printf("PAIRING_COMPLETE\n");
                    break;
                }
                case RFCOMM_EVENT_INCOMING_CONNECTION:
                    rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr);
                    rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
                    rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
                    printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
                    rfcomm_accept_connection(rfcomm_channel_id);
                    break;
               
                case RFCOMM_EVENT_CHANNEL_OPENED:
                    if (rfcomm_event_channel_opened_get_status(packet)) {
                        printf("RFCOMM channel open failed, status %u\n", rfcomm_event_channel_opened_get_status(packet));
                    } else {
                        rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                        mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                        printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
                    }
                    break;
                case RFCOMM_EVENT_CAN_SEND_NOW:
                    // printf("RFCOMM can send now\n");
                    if (lineBuffer)
                    {
                        rfcomm_send(rfcomm_channel_id, (uint8_t*)lineBuffer, (uint16_t)strlen((const char*)lineBuffer));  
                        lineBuffer = 0;
                    }
                    break;
                case RFCOMM_EVENT_CHANNEL_CLOSED:
                    printf("RFCOMM channel closed\n");
                    rfcomm_channel_id = 0;
                    break;
                
                default:
                    break;
            }
            break;
        }
        case RFCOMM_DATA_PACKET:
        {
            // printf("RFCOMM_DATA_PACKET\n");

            for (i = 0; i < size; i++)
            {
                queue_add_blocking(&BlueTooth_Receive_Queue, &packet[i]);
            }
            break;
        }
        default:
            printf("Unknown packet type 0x%X\n", packet_type);
            break;
    }
}


PRIVATE void btstack_main()
{
    spp_service_setup();

    gap_discoverable_control(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_set_local_name("StarLight 00:00:00:00:00:00");

    hci_power_control(HCI_POWER_ON);    // turn on!
}


PRIVATE int bluetooth_main() 
{
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) { printf("Bluetooth init failed\n"); return -1; }
    
    btstack_main();    // run the app
    btstack_run_loop_execute();
}

PUBLIC void BlueTooth_Send_String(char* str)
{
    lineBuffer = str;
    rfcomm_request_can_send_now_event(rfcomm_channel_id);
}

// ----------------------------------------------------------------------------------------

PRIVATE void BlueTooth_Server(void)   // This is the main for the second core.
{
    queue_init(&BlueTooth_Receive_Queue, sizeof(uint8_t), QUEUE_SIZE);
    bluetooth_main();
}

PUBLIC void Start_BlueTooth_Server(void)
{
    multicore_lockout_victim_init();
    multicore_launch_core1(BlueTooth_Server);
}



PUBLIC char BlueTooth_GetChar()
{
    char val;
    queue_remove_blocking(&BlueTooth_Receive_Queue, &val);
    return val;
}


PUBLIC bool BlueTooth_Check_Receive(void)
{
    int val;
    return queue_try_peek(&BlueTooth_Receive_Queue, &val);
}


#include <stdarg.h>
PUBLIC void BlueTooth_Printf(const char *fmt, ...) 
{ // Needs to be fixed.  Change to use a static buffer and then BlueTooth_Send_String
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);    //vsnprintf BlueTooth_Send_String
    va_end(args);
}


// EndFile: bluetooth_stdio.c
