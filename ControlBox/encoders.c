
#include "common.h"
#include "encoders.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
//
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/qei.h"
#include "driverlib/sysctl.h"

#include "controlbox.h"

//set the value of the maximum number of pulses per revolution
#define MAX_PULSES 3999


PRIVATE void UnlockGPIOPin(uint32_t portBase, uint8_t pinMask)
{
    // Unlock port pins.
    //
    HWREG(portBase + GPIO_O_LOCK)= GPIO_LOCK_KEY;
    HWREG(portBase + GPIO_O_CR) |= pinMask;        // unlock pin
}


PRIVATE void Qei_device_init(int n)
{
    int device;

    if (n)
    {
        // GPIO PC5 -> PhA1, PC6 -> PhB1

        GPIOPinTypeQEI(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_5);
        GPIOPinConfigure(GPIO_PC5_PHA1);
        GPIOPinConfigure(GPIO_PC6_PHB1);

        device = QEI1_BASE;
    }
    else
    {
        // GPIO PD6 -> PhA0, PD7 -> PhB0

        UnlockGPIOPin(GPIO_PORTD_BASE, GPIO_PIN_7);   // Unlock PD7

        GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_7 | GPIO_PIN_6);
        GPIOPinConfigure(GPIO_PD6_PHA0);
        GPIOPinConfigure(GPIO_PD7_PHB0);

        device = QEI0_BASE;
    }

    QEIConfigure(device, ( QEI_CONFIG_CAPTURE_A
                         | QEI_CONFIG_QUADRATURE
                         | QEI_CONFIG_RESET_IDX
                         | QEI_CONFIG_NO_SWAP), MAX_PULSES);

    QEIFilterConfigure(device, QEI_FILTCNT_17);
    QEIFilterEnable(device);
    QEIEnable(device);        //Enables the QEI module
}


void Qei_init(void)
{
    Qei_device_init(0);
    Qei_device_init(1);
}


int Qei_GetPos(int n)
{
    return QEIPositionGet(n ? QEI1_BASE : QEI0_BASE);    // Read Encoder.
}


PUBLIC void Qei_Process(void)
{
    static int old_x = 0;
    static int old_y = 0;

    int x = Qei_GetPos(0);

    if (x != old_x)
    {
        char ch = old_x < x ? '^' : 'v';
        printf("X=%d(%c)\n", x, ch);
        CB_Post_Event(CB_TYPE_ENC, 1, x);
        old_x = x;
    }

    int y = Qei_GetPos(1);

    if (y != old_y)
    {
        char ch = old_y < y ? '^' : 'v';
        printf("\tY=%d(%c)\n", y, ch);
        CB_Post_Event(CB_TYPE_ENC, 2, y);
        old_y = y;
    }
}


// EndFile: Encoders.c
