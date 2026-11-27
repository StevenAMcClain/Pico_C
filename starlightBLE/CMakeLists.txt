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
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
# include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(starlight C CXX ASM)

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

add_executable(starlight 
    starlightBLE.c
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
    shell.c
    notifier.c
    matcher.c
    parser.c
    commands.c
    picopins.c
    picopio.c
    picopwm.c
    config.c
    error.c
    debug_pin.c
    parms.c
    ws2812.c
    picoflash.c
    hashtab.c
    )

# add_executable(starlight 
#     starlight.c btstdio.c Led.c ws2812.c blob.c beng.c bcmd.c bvar.c bvar_internal.c matcher.c morph.c 
#     parser.c stack.c obled.c scene.c blob_dump.c shifter.c flashblob.c trace.c spi.c spi_map.c picopins.c picopwm.c linequeue.c)

# Get all C and C++ files, when the glob value changes, cmake will run again and update the files
# file(GLOB_RECURSE app_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.c" "${SOURCE_FOLDER}/*.cpp" )

# -- Added by steven
#pico_set_linker_script(starlight ${CMAKE_SOURCE_DIR}/pico_flash_region_protect.ld)
#set_target_properties(starlight PROPERTIES PICO_TARGET_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/build/pico_flash_region_protect.ld)
# -- End Added by steven

#include_directories($ENV{PICO_SDK_PATH})
pico_set_program_name(starlight "starlight")
pico_set_program_version(starlight "0.1")

# Generate PIO header
pico_generate_pio_header(starlight ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(starlight 1)
pico_enable_stdio_usb(starlight 0)

# add_library(bluetooth OBJECT bluetooth.c)
# add_library(bluetooth_stdio OBJECT bluetooth_stdio.c)

# Add the standard library to the build
# target_link_libraries(starlight
#         bluetooth
#         bluetooth_stdio)

set_target_properties(starlight PROPERTIES PICO_TARGET_LINKER_SCRIPT 
    ${CMAKE_SOURCE_DIR}/starlight.ld)

# Add the standard include files to the build
target_include_directories(starlight PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(starlight 
        hardware_gpio
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_spi
        hardware_pwm 
        hardware_adc
        pico_platform
        pico_malloc 
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_lwip_threadsafe_background
        pico_multicore
        )
#        pico_cyw43_arch_none


pico_add_extra_outputs(starlight)

