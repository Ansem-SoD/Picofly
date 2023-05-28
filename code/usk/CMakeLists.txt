# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE MinSizeRel)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(usk C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(usk main.c board_detect.c fuses.c pio_upload.c boot_detect.c config.c misc.c payload.c glitch.c)

target_compile_definitions(usk PRIVATE PICO_NO_BINARY_INFO)

pico_generate_pio_header(usk ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(usk ${CMAKE_CURRENT_LIST_DIR}/emmc.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

pico_set_program_name(usk "usk")
pico_set_program_version(usk "2.7")

pico_set_binary_type(usk no_flash)

pico_enable_stdio_uart(usk 0)
pico_enable_stdio_usb(usk 0)

# Add the standard library to the build
target_link_libraries(usk pico_stdlib)

# Add any user requested libraries
target_link_libraries(usk
        hardware_pio
        hardware_adc
        hardware_flash
        )

pico_add_extra_outputs(usk)

