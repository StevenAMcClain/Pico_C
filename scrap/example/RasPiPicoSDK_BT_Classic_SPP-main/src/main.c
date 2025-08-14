/*
 * Copyright (c) 2022 Mr. Green's Workshop https://www.MrGreensWorkshop.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdio.h>

#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1

#include "pico/cyw43_arch.h"
#include <pico/stdlib.h>
#include <btstack_run_loop.h>

int btstack_main(int argc, const char * argv[]);

int main() {
    stdio_init_all();

    // initialize CYW43 driver
    if (cyw43_arch_init()) 
    {
        printf("cyw43_arch_init() failed.\n");
        return -1;
    }

    // run the app
    btstack_main(0, NULL);
    btstack_run_loop_execute();
}
