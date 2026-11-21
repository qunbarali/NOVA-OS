cmake_minimum_required(VERSION 3.13)
if(NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Install the Raspberry Pi Pico SDK and set PICO_SDK_PATH.")
endif()
include(pico_sdk_import.cmake)
project(nova_os VERSION 1.5.0 LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
option(NOVA_DISPLAY_ILI9341 "Use an ILI9341 panel instead of the default ST7735 profile" OFF)
pico_sdk_init()
add_executable(nova_os
    src/main.c
    src/kernel/kernel.c
    src/kernel/task.c
    src/kernel/crash.c
    src/kernel/critical.c
    src/kernel/smp_lock.c
    src/kernel/device.c
    src/kernel/storage_owner.c
    src/kernel/memory.c
    src/kernel/ipc.c
    src/hal/board.c
    src/hal/display.c
    src/hal/font.c
    src/ui/launcher.c
    src/hal/battery.c
    src/hal/diskio.c
    src/hal/fs.c
    src/usb/usb.c
    src/usb/usb_descriptors.c
    third_party/fatfs/ff.c
    src/hal/input.c
    src/hal/spi_bus.c
    src/hal/sd_spi.c
    src/shell/shell.c
)
if(PICO_PLATFORM STREQUAL "rp2040")
    target_sources(nova_os PRIVATE src/arch/armv6m/call_on_stack.S src/arch/armv6m/fault_entry.S)
    target_compile_definitions(nova_os PRIVATE NOVA_ARCH_ARMV6M=1)
elseif(PICO_PLATFORM STREQUAL "rp2350-arm-s")
    target_sources(nova_os PRIVATE src/arch/armv8m/call_on_stack.S src/arch/armv8m/fault_entry.S)
    target_compile_definitions(nova_os PRIVATE NOVA_ARCH_ARMV8M=1)
else()
    message(FATAL_ERROR "NOVA currently supports RP2040 and RP2350 Cortex-M33 only; got PICO_PLATFORM=${PICO_PLATFORM}")
endif()
target_include_directories(nova_os PRIVATE include third_party/fatfs)
if(NOVA_DISPLAY_ILI9341)
    target_compile_definitions(nova_os PRIVATE NOVA_DISPLAY_ILI9341=1)
endif()
target_compile_options(nova_os PRIVATE -Wall -Wextra -Wshadow -Wconversion -Wstrict-prototypes)
target_link_libraries(nova_os PRIVATE pico_stdlib pico_multicore hardware_gpio hardware_timer hardware_clocks hardware_watchdog hardware_sync hardware_spi hardware_adc tinyusb_device)
pico_enable_stdio_usb(nova_os 0)
pico_enable_stdio_uart(nova_os 0)
pico_enable_stdio_uart(nova_os 0)
pico_add_extra_outputs(nova_os)
message(STATUS "NOVA OS version: ${PROJECT_VERSION}")
message(STATUS "NOVA OS board: ${PICO_BOARD}")
message(STATUS "NOVA OS platform: ${PICO_PLATFORM}")
