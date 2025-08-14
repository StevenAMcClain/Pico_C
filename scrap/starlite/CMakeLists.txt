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
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(starlite C CXX ASM)

# Set top source code folder
set(SOURCE_FOLDER "src")

# Set build type (options: 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo')
set(default_build_type "Debug")

# Set CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build, options are: 'Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo'." FORCE)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



# Add an executable target for the project
add_executable(${PROJECT_NAME})

# Get all the pio files, when the glob value changes, cmake will run again and update the files
file(GLOB_RECURSE pio_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.pio")

# Convert List to String
STRING(REPLACE ";" "\n" PIO_SRC_STR "${pio_src}")

# Print the pio source file list
message(STATUS "PIO code source files: \n${PIO_SRC_STR}")

# If there are any PIO source files, include them to the build.
if (NOT pio_src STREQUAL "")
  pico_generate_pio_header(${PROJECT_NAME} ${pio_src})
endif()

# Get all C and C++ files, when the glob value changes, cmake will run again and update the files
file(GLOB_RECURSE app_src CONFIGURE_DEPENDS "${SOURCE_FOLDER}/*.c" "${SOURCE_FOLDER}/*.cpp" )

# Convert List to String
STRING(REPLACE ";" "\n" APP_SRC_STR "${app_src}")

# Print the C and C++ source file list
message(STATUS "C and C++ source files: \n${APP_SRC_STR}")

# Add C and C++ source files to the build
target_sources(${PROJECT_NAME} PRIVATE ${app_src})

# Link the Project to extra libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_multicore
  pico_btstack_classic
  pico_btstack_cyw43
  pico_cyw43_arch_lwip_threadsafe_background
)

# Including header files directly from project directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
    inc
)

# Export binaries like hex, bin, and uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

# Enable or Disable UART
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Enable or Disable USB CDC
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Disable DTR check for USB CDC connection
# add_definitions(-DPICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1)

# Add executable. Default name is the project name, version 0.1
#add_executable(starlite starlite.c )

#pico_set_program_name(starlite "starlite")
#pico_set_program_version(starlite "0.1")

# Modify the below lines to enable/disable output over UART/USB
#pico_enable_stdio_uart(starlite 1)
#pico_enable_stdio_usb(starlite 0)

# Add the standard library to the build
#target_link_libraries(starlite
#        pico_stdlib)

# Add the standard include files to the build
#target_include_directories(starlite PRIVATE
#        ${CMAKE_CURRENT_LIST_DIR}
#)

# Add any user requested libraries
#target_link_libraries(starlite 
#        hardware_dma
#        hardware_timer
#        hardware_watchdog
#        pico_cyw43_arch_none
#        pico_btstack_cyw43
#        )

# pico_add_extra_outputs(starlite)

