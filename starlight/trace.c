// File: Trace.c

#include "Common.h"

#include "Trace.h"

#include <pico/stdlib.h>

#define TRACE_PIN 20


PUBLIC void Trace_Init()
{
    gpio_init(TRACE_PIN);
    gpio_set_dir(TRACE_PIN, GPIO_OUT);
}


PUBLIC void Trace_Start()
{
    gpio_put(TRACE_PIN, 1);
}


PUBLIC void Trace_End()
{
    gpio_put(TRACE_PIN, 0);
}

// EndFile: Trace.c
