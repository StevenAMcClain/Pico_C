// File: adc.h

//#define USE_PINTRACE
//#define USE_PRINTF

#include "common.h"
#include "adc.h"

#ifdef USE_PRINTF
#include <stdio.h>
#endif
#include <stdint.h>
#include <stdbool.h>
//
#include "inc/hw_memmap.h"
//
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
//
#include "controlbox.h"

#ifdef USE_PINTRACE
#include "pintrace.h"
#endif

#define NUM_ADC_VALUES 3
#define MAX_ADC_VALUES 8

#define MAX_ADC 4095

#define ADC_SEQUENCE 2   // ADC Sequencer to use.


PUBLIC void Adc_Init(void)
{
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    ADCHardwareOversampleConfigure(ADC0_BASE, 64);

    ADCSequenceConfigure(ADC0_BASE, ADC_SEQUENCE, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE, 0, ADC_CTL_CH2);
    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE, 1, ADC_CTL_CH1);
    ADCSequenceStepConfigure(ADC0_BASE, ADC_SEQUENCE, 2, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, ADC_SEQUENCE);
}


PRIVATE void Adc_Sample(uint32_t *ADCValues)
{
    ADCIntClear(ADC0_BASE, ADC_SEQUENCE);                       // Clear the interrupt status flag.

    ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCE);              // Trigger the ADC conversion.

    while(!ADCIntStatus(ADC0_BASE, ADC_SEQUENCE, false)) { }   // Wait for conversion to be completed.

    ADCIntClear(ADC0_BASE, ADC_SEQUENCE);                      // Clear the ADC interrupt flag.

    ADCSequenceDataGet(ADC0_BASE, ADC_SEQUENCE, ADCValues);    // Read ADC Value.
}


PRIVATE volatile int is_running_count[MAX_ADC_VALUES] = {0};
PRIVATE volatile bool is_running[MAX_ADC_VALUES] = {0};

PUBLIC void Adc_Tick(void)
{
    volatile int* p = is_running_count;

    for (int i = 0; i < NUM_ADC_VALUES; ++i, ++p)
    {
        if (*p && --(*p) == 0)
        {
            is_running[i] = false;
#ifdef USE_PINTRACE
            PinTrace_Clear();
#endif
       }
    }
}

//#define FIXED_THRESH

#define RUNNING_THRESH 1
#define STOPPING_THRESH 10
#define STOPPED_THRESH 15

#ifndef FIXED_THRESH
int running_thresh = RUNNING_THRESH;
int stopping_thresh = STOPPING_THRESH;
int stopped_thresh = STOPPED_THRESH;

#undef RUNNING_THRESH
#undef STOPPING_THRESH
#undef STOPPED_THRESH

#define RUNNING_THRESH running_thresh
#define STOPPING_THRESH stopping_thresh
#define STOPPED_THRESH stopped_thresh
#endif

#define IS_RUNNING_TIME 10  // ms


PUBLIC void Adc_Process(void)
{
    static uint32_t old[MAX_ADC_VALUES];
    uint32_t ADCValues[MAX_ADC_VALUES];
#ifdef USE_PRINTF
    bool is_first = true;
#endif
    Adc_Sample(ADCValues);

    for (int i= 0; i < NUM_ADC_VALUES; ++i)
    {
        int v = (MAX_ADC - ADCValues[i]);

        int d = v - old[i];
        d = ABS(d);

        int thresh = is_running[i] ? RUNNING_THRESH : STOPPED_THRESH;

        if (d > thresh)
        {
            old[i] = v;

            is_running[i] = true;

#ifdef USE_PINTRACE
            PinTrace_Set();
#endif
            if (d > STOPPING_THRESH)
            {
                is_running_count[i] = IS_RUNNING_TIME;
            }

#ifdef USE_PRINTF
            char pad[] = "\t\t\t\t\t";

            if (is_first) { is_first = false; pad[i + 2] = 0; } else { pad[0] = 0; }

            printf("%sA%d=%d\t", pad, i, v);
#endif
            CB_Post_Event(CB_TYPE_POT, 1 + i, v);
        }
#ifdef USE_PRINTF
        else { if (!is_first) { printf("\t"); } }
#endif
    }

#ifdef USE_PRINTF
    if (!is_first) { printf("\n"); }
#endif
}


// EndFile: adc.c
