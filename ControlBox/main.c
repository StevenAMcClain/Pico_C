// File: main.c    -- Start of ControlBox
//

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef USE_PRINTF
#include <stdio.h>
#endif

#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

#ifdef USE_UARTPRINTF
#include "utils/uartstdio.h"
#endif

#include "adc.h"
#include "buttons.h"
#include "comport.h"
#include "encoders.h"
#include "pintrace.h"

#include "controlbox.h"

#define PIN_LOW 0
#define PIN_HIGH 1

volatile uint64_t now = 0;


PRIVATE void SysTick_ISR(void)
{
    ++now;
    Adc_Tick();
}


PRIVATE void Systick_Init(void)
{
    SysTickPeriodSet(SysCtlClockGet() / 1000);  // 1 ms tick
    SysTickIntRegister(SysTick_ISR);
    SysTickEnable();
    SysTickIntEnable();
}


#ifdef USE_UARTPRINTF
PRIVATE void UART_Init(void)
{
    // Configure GPIO for UART0 (PA0 and PA1)
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART0 with 115200 baud rate
    UARTStdioConfig(0, 115200, SysCtlClockGet());
}
#endif // USE_UARTPRINTF


PUBLIC void delayMs(uint32_t ui32Ms)
{
    // 1 clock cycle = 1 / SysCtlClockGet() second
    // 1 SysCtlDelay = 3 clock cycle = 3 / SysCtlClockGet() second
    // 1 second = SysCtlClockGet() / 3
    // 0.001 second = 1 ms = SysCtlClockGet() / 3 / 1000

    SysCtlDelay(ui32Ms * (SysCtlClockGet() / 3 / 1000));
}


PRIVATE void start_clocks(void)
{
    // Set the clock to run at 40MHz
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

    // Enable GPIO devices.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Enable other devices.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2)) { continue; }
}


#define handle_button(but, evt)\
if (pui8Delta & (but))\
{\
    int v = BUTTON_PRESSED((but), pui8Delta, pui8State) ? CB_PB_PRESSED : CB_PB_RELEASED;\
    CB_Post_Event(CB_TYPE_PB, (evt), v);\
}

PRIVATE void Button_Update(void)
{
    static uint8_t last_State = 0;

    uint8_t pui8Delta = 0;
    uint8_t pui8State = Buttons_Poll(&pui8Delta, NIL);

    if (last_State != pui8State)
    {
        last_State = pui8State;

#ifdef USE_PRINTF
        printf("Buttons: Delta %d, State %d\n", pui8Delta, pui8State);
#endif
        handle_button(L_ENCBUT, CB_PB_LENC);
        handle_button(R_ENCBUT, CB_PB_RENC);

        handle_button(L_BUTTON, CB_PB_LPB);
        handle_button(R_BUTTON, CB_PB_RPB);

        handle_button(SWITCH, CB_PB_SW);
    }
}

// -------------------------------

typedef struct
{
    uint32_t ui32Port;
    uint8_t ui8Pins;

} UNUSED_GPIO_PINS;


#ifdef USE_UARTPRINTF
#define UNUSED_GPIO_A 0xFC
#else
#define UNUSED_GPIO_A 0xFF
#endif

#define UNUSED_GPIO_B 0xD0
#define UNUSED_GPIO_C 0x90
#define UNUSED_GPIO_D 0xC0
#define UNUSED_GPIO_E 0xD1
#define UNUSED_GPIO_F 0xE0

//USE_PINTRACE

UNUSED_GPIO_PINS Unused_Gpio_Pins[] =
{
    {GPIO_PORTA_BASE, UNUSED_GPIO_A},
    {GPIO_PORTB_BASE, UNUSED_GPIO_B},
    {GPIO_PORTC_BASE, UNUSED_GPIO_C},
    {GPIO_PORTD_BASE, UNUSED_GPIO_D},
    {GPIO_PORTE_BASE, UNUSED_GPIO_E},
    {GPIO_PORTF_BASE, UNUSED_GPIO_F},
};


PRIVATE void setup_unused_ports(void)
{
    UNUSED_GPIO_PINS* ugp = Unused_Gpio_Pins;

    for (int i = 0; i < ARRAY_SIZE(Unused_Gpio_Pins); ++i, ++ugp)
    {
        GPIOPinTypeGPIOOutput(ugp->ui32Port, ugp->ui8Pins);
        GPIOPinWrite(ugp->ui32Port, ugp->ui8Pins, 0);
    }
}


PUBLIC void main(void)
{
    start_clocks();

    setup_unused_ports();

#ifdef USE_PINTRACE
    PinTrace_Init();
#endif

#ifdef USE_UARTPRINTF
    UART_Init();
#endif
    SSI_Init_Master();

    Adc_Init();
    Buttons_Init();
    Qei_init();

    Systick_Init();

#ifdef USE_UARTPRINTF
    UARTprintf("ControlBox %s\n", VERSION);
#endif
#ifdef USE_PRINTF
    printf("ControlBox %s\n", VERSION);
#endif

    while (1)
    {
        Adc_Process();
        Button_Update();
        Qei_Process();
        delayMs(10);
    }

//    uint32_t counter = 0;


//    while (1)
//    {
//        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
//        SSI_sendData(counter++);
//        delayMs(1);
//        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
//        delayMs(249);
//    }
}
