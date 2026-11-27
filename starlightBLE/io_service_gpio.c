// File: io_service_gpio.c

#include "common.h"
#include "io_service_gpio.h"

#include <stdatomic.h>
#include <stdio.h>

#include <hardware/gpio.h>

#include <btstack.h>

#include "error.h"
#include "notifier.h"
#include "picopins.h"

PRIVATE uint8_t iunit = 0;
PRIVATE uint8_t ounit = 0;

PRIVATE ATT_VALUE* pin_handle[PICOPIN_MAX + 1] = {0};

PRIVATE atomic_uint_fast32_t last_gpio_state = 0;

PRIVATE uint32_t last_change_time[PICOPIN_MAX+1];
PRIVATE uint8_t debounce_ms[PICOPIN_MAX+1];
PRIVATE uint8_t hysteresis_ms[PICOPIN_MAX+1];

typedef struct {
    uint8_t pin;
    bool rising;
    uint32_t timestamp;
} gpio_event_t;

#define EVENT_QUEUE_SIZE 32
PRIVATE gpio_event_t event_queue[EVENT_QUEUE_SIZE];
PRIVATE volatile uint8_t event_head = 0;
PRIVATE volatile uint8_t event_tail = 0;

static inline void push_event(uint8_t pin, bool rising, uint32_t ts)
{
    uint8_t next = (event_head + 1) % EVENT_QUEUE_SIZE;
    if (next != event_tail) {
        event_queue[event_head].pin = pin;
        event_queue[event_head].rising = rising;
        event_queue[event_head].timestamp = ts;
        event_head = next;
    }
}

PRIVATE bool set_gpio_state(uint8_t pin, bool state)
{
    if (pin > PICOPIN_MAX) return false;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t mask = (1u << pin);

    uint32_t prev = state ?
        atomic_fetch_or(&last_gpio_state, mask) :
        atomic_fetch_and(&last_gpio_state, ~mask);

    bool was_set = (prev & mask);
    bool rising  = state && !was_set;
    bool falling = !state && was_set;

    if (!(rising || falling)) {
        // Bounce: no state change
        return false;
    }

    // Hysteresis: ignore rapid toggles after a valid edge
    if (now - last_change_time[pin] < hysteresis_ms[pin]) {
        return false;
    }

    // Debounce: require stable state for debounce_ms
    if (now - last_change_time[pin] < debounce_ms[pin]) {
        return false;
    }

    // Valid edge
    last_change_time[pin] = now;
    push_event(pin, rising, now);
    return true;
}

// PRIVATE volatile uint32_t last_gpio_state = 0;

// PRIVATE bool set_gpio_state(uint8_t pin, bool state)
// {
//     if (pin > PICOPIN_MAX) return false;

//     uint32_t mask = (1u << pin);

//     if (state)
//     {
//         // Atomically set the bit, return previous value
//         uint32_t prev = atomic_fetch_or(&last_gpio_state, mask);

//         if (!(prev & mask))
//         {
//             printf("%s: gpio %d, up.                    !!!^^^ UP ^^^!!!\n", __func__, pin);
//             return true;        // rising edge
//         }
//         else
//         {
//             printf("%s: gpio %d, bounce up.\n", __func__, pin);
//             return false;
//         }
//     }
//     else
//     {
//         // Atomically clear the bit, return previous value
//         uint32_t prev = atomic_fetch_and(&last_gpio_state, ~mask);

//         if (prev & mask)
//         {
//             printf("%s: gpio %d, down.                    !!!vvv DOWN vvv!!!\n", __func__, pin);
//             return true;        // falling edge
//         }
//         else
//         {
//             printf("%s: gpio %d, bounce down.\n", __func__, pin);
//             return false;
//         }
//     }
// }

// PRIVATE bool set_gpio_state(uint8_t pin, bool state)
// {
//     bool result = false;

//     if (pin <= PICOPIN_MAX)
//     {
//         uint32_t mask = (1 << pin);

//         if (state)
//         {
//             if ( !(last_gpio_state & mask) )
//             {
//                 printf("%s: gpio %d, up.\n", __func__, pin);
//                 last_gpio_state |= mask;        // Set state.
//                 result= true;
//             }
//             else { printf("%s: gpio %d, bounce up.\n", __func__, pin); }
//         }
//         else
//         {
//             if (last_gpio_state & mask)
//             {
//                 printf("%s: gpio %d, down.\n", __func__, pin);
//                 last_gpio_state &= ~mask;        // Clear state.
//                 result= true;
//             }
//             else { printf("%s: gpio %d, bounce down.\n", __func__, pin); }
//         }
//     }
//     return result;
// }


PRIVATE uint16_t IOSERV_Read_GPIO(ATT_VALUE* att_value, uint8_t* value, uint16_t value_len)
{
    uint16_t rval = 0;

    if (att_value->iohandle)
    {
        if (value)
        {
            int pin = (int)att_value->iohandle;
            int val = gpio_get(pin);

            set_gpio_state(pin, val);
            
            if (value_len >= 1) 
            {
                value[0] = val; 
            }
            else { POST_ERROR("value buffer too small(%p, %d)!", value, value_len); }
        }
        rval = 1;   // One byte value.
    }
    else { POST_ERROR("no io handle!"); }

    return rval;
}


PRIVATE bool IOSERV_Write_GPIO(ATT_VALUE* att_value, uint32_t val)
{
    if (att_value->iohandle)
    {
        int pin = (uint)att_value->iohandle;

        gpio_put(pin, val);
        set_gpio_state(pin, val);

        return true;
    }
    else { POST_ERROR("%s: no io handle!", __func__); }

    return false;
}


PRIVATE void gpio_callback(uint port_pin, uint32_t events) //The GPIO callback function to handle interrupts
{
    bool state = (events & GPIO_IRQ_EDGE_FALL) ? false : true;

    if (set_gpio_state(port_pin, state))
    {
        printf("#: %s: gpio %d, events %ld\n", __func__, port_pin, events);

        if (connection_handle != HCI_CON_HANDLE_INVALID)    // Echo back via TX notify
        {
            if (port_pin <= PICOPIN_MAX)
            {
                uint32_t mask = (1 << port_pin);
                uint32_t snapshot = atomic_load(&last_gpio_state);
                bool val = (snapshot & mask) ? 1 : 0;

                ATT_VALUE* att_val = pin_handle[port_pin];

                if (att_val)
                {
                    static const char bit[] = {0,1};       // Fixed buff for notify.
                    Notifier_Post_buffer(att_val->handle, &bit[val], sizeof(uint8_t));
                }
            }
            else { POST_ERROR("Bad pin number %d!\n", port_pin); }
        }
    }
//    else { printf("gpio %d, bounce.\n", port_pin); }
}


PRIVATE bool IOSERV_Initialize_GPIO_Input(const char* name, uint8_t* port_pins, size_t pin_count, const char* user_desc)
{
    UNUSED(pin_count);

    uint8_t port_pin = *port_pins;

    if (PicoPin_Reserve(&Pico_Pins, port_pin))
    {
        gpio_init(port_pin);
        gpio_set_dir(port_pin, GPIO_IN);    // input on GPIO PORT_PIN

        gpio_set_irq_enabled_with_callback(port_pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

        handle_t handle = New_Characteristic(name, READ_ONLY, IOTYPE_INPUT, iunit++);
 
        ATT_VALUE* val = Att_Values_Add(handle, name, IOTYPE_INPUT, (uintptr_t)port_pin, 0, 0, user_desc);

        pin_handle[port_pin] = val;

        return true;
    }
    else { POST_ERROR("port pin %d is already reserved!", port_pin); }

    return false;
}


PRIVATE bool IOSERV_Initialize_GPIO_Output(const char* name, uint8_t* port_pins, size_t pin_count, const char* user_desc)
{
    UNUSED(pin_count);

    uint8_t port_pin = *port_pins;
    
    if (PicoPin_Reserve(&Pico_Pins, port_pin))
    {
        handle_t handle = New_Characteristic(name, CAN_WRITE, IOTYPE_OUTPUT, ounit++);

        gpio_init(port_pin);
        gpio_set_dir(port_pin, GPIO_OUT);    // output on GPIO PORT_PIN

        ATT_VALUE* val = Att_Values_Add(handle, name, IOTYPE_OUTPUT, (uintptr_t)port_pin, 0, 0, user_desc);

        pin_handle[port_pin] = val;

        return true;
    }
    else { POST_ERROR("Can't reserve port pin %d!", port_pin); }

    return false;
}


PUBLIC ioserv_function_t IOServ_GPIO_Output = 
{
    "OUTPUT",
    IOSERV_Initialize_GPIO_Output,
    IOSERV_Read_GPIO,
    IOSERV_Write_GPIO
};


PUBLIC ioserv_function_t IOServ_GPIO_Input = 
{
    "INPUT",
    IOSERV_Initialize_GPIO_Input,
    IOSERV_Read_GPIO,
    NULL
};


// End of file: io_service_gpio.c
