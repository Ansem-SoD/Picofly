# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(busk C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)

add_executable(busk main.c crc.c)

target_compile_definitions(busk PRIVATE PICO_NO_BINARY_INFO)

pico_set_program_name(busk "busk")
pico_set_program_version(busk "2.7")

pico_set_binary_type(busk default)

pico_enable_stdio_uart(busk 0)
pico_enable_stdio_usb(busk 0)

# Add the standard library to the build
target_link_libraries(busk pico_stdlib)

pico_set_boot_stage2(busk slower_boot2)

pico_add_extra_outputs(busk)

