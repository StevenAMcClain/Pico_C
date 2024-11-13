#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/sync.h"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "pico/cyw43_arch.h"
#include <pico/multicore.h>



// Data will be copied from src to dst
const char src[] = "Hello, world! (from DMA)";
char dst[count_of(src)];

#include "blink.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}


#define QUEUE_SIZE 100
queue_t BlueTooth_Receive;

// void x_server()   // This is the main for the second core.
// {
//     multicore_lockout_victim_init();
//     while (1);
// }

void bluetooth_server(void)   // This is the main for the second core.
{
    queue_init(&BlueTooth_Receive, sizeof(uint8_t), QUEUE_SIZE);
    extern int bt_main();
    bt_main();
}

void bluetooth_transmit(char* str)
{
    void BlueTooth_Send_String(char* str);
    BlueTooth_Send_String(str);
}

char bluetooth_receive()
{
    char val;
    queue_remove_blocking(&BlueTooth_Receive, &val);
    return val;
}

bool bluetooth_check_receive()
{
    char val;
    return queue_try_peek(&BlueTooth_Receive, &val);
}


int main()
{
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
    
    multicore_lockout_victim_init();

    stdio_init_all();

    printf("\r\n\nStartlight %s %s %s: startup.\n", 
                __VERSION__ , __TIME__, __DATE__);

    multicore_launch_core1(bluetooth_server);
 ///   bluetooth_server();

    // // Get a free channel, panic() if there are none
    // int chan = dma_claim_unused_channel(true);
    
    // // 8 bit transfers. Both read and write address increment after each
    // // transfer (each pointing to a location in src or dst respectively).
    // // No DREQ is selected, so the DMA transfers as fast as it can.
    
    // dma_channel_config c = dma_channel_get_default_config(chan);
    // channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    // channel_config_set_read_increment(&c, true);
    // channel_config_set_write_increment(&c, true);
    
    // dma_channel_configure(
    //     chan,          // Channel to be configured
    //     &c,            // The configuration we just created
    //     dst,           // The initial write address
    //     src,           // The initial read address
    //     count_of(src), // Number of transfers; in this case each is 1 byte.
    //     true           // Start immediately.
    // );
    
    // // We could choose to go and do something else whilst the DMA is doing its
    // // thing. In this case the processor has nothing else to do, so we just
    // // wait for the DMA to finish.
    // dma_channel_wait_for_finish_blocking(chan);
    
    // // The DMA has now copied our text from the transmit buffer (src) to the
    // // receive buffer (dst), so we can print it out from there.
    // puts(dst);

    // // PIO Blinking example
    // PIO pio = pio0;
    // uint offset = pio_add_program(pio, &blink_program);
    // printf("Loaded program at %d\n", offset);
    
    // #ifdef PICO_DEFAULT_LED_PIN
    // blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
    // #else
    // blink_pin_forever(pio, 0, offset, 6, 3);
    // #endif
    // // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    // // Timer example code - This example fires off the callback after 2000ms
    // add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    while (true) 
    {
        printf("Hello, world!\n");
        sleep_ms(1000);

        {
            static int x = 0;
            char buffer[100];
            sprintf(buffer, "Count %d\n", x++);
            bluetooth_transmit(buffer);
        }

        if (bluetooth_check_receive())
        {
            printf("BlueTooth_RX: '");
            while (bluetooth_check_receive())
            {
                char val = bluetooth_receive();
                putchar(val);
            }
            printf("'\n");
        }
    }
}
