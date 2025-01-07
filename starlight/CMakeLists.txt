# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(starlight C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# Add executable. Default name is the project name, version 0.1

add_executable(starlight starlight.c bluetooth_stdio.c Led.c ws2812.c blob.c matcher.c morph.c parser.c stack.c obled.c scene.c blob_dump.c)

# Get all C and C++ files, when the glob value changes, cmake will run again and update the files
# file(GLOB_RECURSE app_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.c" "${SOURCE_FOLDER}/*.cpp" )

# -- Added by steven
#pico_set_linker_script(starlight ${CMAKE_SOURCE_DIR}/pico_flash_region_protect.ld)
set_target_properties(starlight PROPERTIES PICO_TARGET_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/build/pico_flash_region_protect.ld)
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

# Add the standard include files to the build
target_include_directories(starlight PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(starlight 
        hardware_dma
        hardware_pio
        hardware_timer
        pico_cyw43_arch_none
        pico_stdlib
        pico_malloc 
        pico_btstack_classic
        pico_btstack_cyw43
        pico_cyw43_arch_none
        pico_multicore
        )


pico_add_extra_outputs(starlight)

