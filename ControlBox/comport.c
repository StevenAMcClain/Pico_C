// File: ComPort.c

// CONNECTIONS:

// PB7 - SSI2Tx    (MOSI2)
// PB6 - SSI2Rx    (MISO2)
// PB5 - SSI2Fss   (CS2)           (actually PE5)  (Active low)
// PB4 - SSI2CLK   (SCK2)

#include <comport.h>
#include "common.h"
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/ssi.c"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"
#include "utils/uartstdio.c"


#ifdef INCLUDE_SLAVE
void SSI_Init_Slave(void)
{
    //
    // The SSI2 peripheral must be enabled for use.
    ///
    SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);

    ///CONFIGURE ALTERNATIVE FUNCTION OF GPIO PIN

    GPIOPinConfigure(GPIO_PB4_SSI2CLK);    // Clock is PB4
    //    GPIOPinConfigure(GPIO_PB5_SSI2FSS);  // Fss is PB5
    GPIOPinConfigure(GPIO_PB6_SSI2RX);     // MISO is PB6
    GPIOPinConfigure(GPIO_PB7_SSI2TX);     // MOSI is PB7

    //    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7);

    SSIConfigSetExpClk(SSI2_BASE,
                        SysCtlClockGet(),        // rate of the clock supplied to the SSI module.
                        SSI_FRF_MOTO_MODE_3,     // freescale SPI mode 3 (Polarity:1, Phase:1)
                        SSI_MODE_SLAVE,          // specifies the mode of operation : MASTER in our case
                        1000000,                 // specifies the clock rate
                        16);                     // specifies number of bits transferred per frame
    SSIEnable(SSI2_BASE);

    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);     // Manual SSI2FSS
}


#define BUFFER_SIZE 8
uint32_t SPI_Slave_Receive(void)
{
    static uint16_t rxBuffer[BUFFER_SIZE];

    uint32_t data;

    for (int i = BUFFER_SIZE - 1; i >= 0; --i)
    {
        while (!SSIDataGetNonBlocking(SSI2_BASE, &data)) {} // Wait for data
        rxBuffer[i] = (uint16_t)data;
    }
    return *(uint32_t*)rxBuffer;
}
#endif // INCLUDE_SLAVE


#ifdef INCLUDE_MASTER
void SSI_Init_Master(void)
{
//
// The SSI2 peripheral must be enabled for use.
///
    SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);

///CONFIGURE ALTERNATIVE FUNCTION OF GPIO PIN

    GPIOPinConfigure(GPIO_PB4_SSI2CLK);    // Clock is PB4
//    GPIOPinConfigure(GPIO_PB5_SSI2FSS);  // Fss is PB5
    GPIOPinConfigure(GPIO_PB6_SSI2RX);     // MISO is PB6
    GPIOPinConfigure(GPIO_PB7_SSI2TX);     // MOSI is PB7
    
//    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7);

    SSIConfigSetExpClk(SSI2_BASE,
                               SysCtlClockGet(),        // rate of the clock supplied to the SSI module.
                               SSI_FRF_MOTO_MODE_3,     // freescale SPI mode 3 (Polarity:1, Phase:1)
                               SSI_MODE_MASTER,         // specifies the mode of operation : MASTER in our case
                               1000000,                 // specifies the clock rate
                               16);                     // specifies number of bits transferred per frame
    SSIEnable(SSI2_BASE);

    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);     // Manual SSI2FSS
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);    // Pulling Fss starts HIGH.
}


void SSI_sendData(uint32_t val)
{
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0); // PULLING CS LOW BEFORE STARTING DATA TRANSMIT

    SSIDataPut(SSI2_BASE, (val >> 16) & 0xFFFF);     // Put high word of data into the SSI transmit FIFO
    SSIDataPut(SSI2_BASE, val & 0xFFFF);             // Put low word of data
    SysCtlDelay(5);
    while(SSIBusy(SSI2_BASE))  { continue; }

    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5); //Setting CS HIGH again.
}
#endif // INCLUDE_MASTER

// EndFile: ComPort.c
