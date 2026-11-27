# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD sparkfun_iotredboard_rp2350 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(StartLightBLE2W C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# --------------------------------------------------------------------------- Start: DISABLE OPTIMIZATIONS for easier debugging.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fno-inline -fno-builtin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fno-inline -fno-builtin")
add_compile_options(-g3 -ggdb -fno-omit-frame-pointer)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
set(PICO_DEFAULT_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE Debug)
# --------------------------------------------------------------------------- End: DISABLE OPTIMIZATIONS for easier debugging.

add_compile_options(-Wall -Wextra)                      # Extra checking for all source files

# Add executable. Default name is the project name, version 0.1

add_executable(StartLightBLE2W 
    StartLightBLE2W.c 
    BLECore.c
    spsc_queue.c
    dev_info_service.c
    nus_service.c
    io_service.c

    io_service_gpio.c
    io_service_pwm.c
    io_service_bus.c
    io_service_led.c
    io_service_strip.c

    globmatch.c
    picoflash.c
    psram.c
    ws2812.c

    error.c
    debug_pin.c

    picopins.c
    picopio.c
    picopwm.c

    balloc.c
    hashtab.c
    heap.c

    shell.c
    config.c

    notifier.c
    watcher.c

    atom.c
    context.c
    operator.c
    lexer.c
    parser.c
    evaluator.c
    print.c
    command.c
    variables.c

    commands/cmd_table.c

    commands/cmd_help.c
    commands/cmd_if.c
    commands/cmd_list.c
    commands/cmd_parms.c
    commands/cmd_print.c
    commands/cmd_return.c
    commands/cmd_srvs.c
    commands/cmd_test.c
    commands/cmd_watch.c
    commands/cmd_misc.c
    var_list.c
    )

pico_set_program_name(StartLightBLE2W "StartLightBLE2W")
pico_set_program_version(StartLightBLE2W "0.1")

# Generate PIO header
pico_generate_pio_header(StartLightBLE2W ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# target_link_options(StartLightBLE2W PRIVATE -T custom.ld)

# set_target_properties(StartLightBLE2W PROPERTIES PICO_TARGET_LINKER_SCRIPT 
#     ${CMAKE_SOURCE_DIR}/StartLightBLE2W.ld)
# pico_add_linker_script(StartLightBLE2W custom.ld)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(StartLightBLE2W 1)
pico_enable_stdio_usb(StartLightBLE2W 0)

# Add the standard include files to the build
target_include_directories(StartLightBLE2W PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/headers
)

# Add any user requested libraries
target_link_libraries(StartLightBLE2W 
        hardware_adc
        hardware_dma
        hardware_gpio
        hardware_flash
        hardware_pio
        hardware_pwm 
        hardware_spi
        hardware_timer
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_lwip_threadsafe_background
        pico_multicore
        pico_platform
        )

#        pico_malloc 

#        pico_cyw43_arch_none


pico_add_extra_outputs(StartLightBLE2W)
