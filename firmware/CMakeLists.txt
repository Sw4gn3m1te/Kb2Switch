cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w CACHE STRING "Board type")
# Picked up by the SDK's TinyUSB integration to create the tinyusb_pico_pio_usb target
set(PICO_PIO_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/Pico-PIO-USB)

# Repo-local toolchain fallback (git-ignored ../.tools), used unless paths are given explicitly
get_filename_component(KB_TOOLS ${CMAKE_CURRENT_LIST_DIR}/../.tools ABSOLUTE)
if (EXISTS ${KB_TOOLS})
    if (NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH ${KB_TOOLS}/pico-sdk)
    endif()
    if (NOT DEFINED PICO_TOOLCHAIN_PATH AND NOT DEFINED ENV{PICO_TOOLCHAIN_PATH})
        set(PICO_TOOLCHAIN_PATH ${KB_TOOLS}/gcc/bin)
    endif()
    if (NOT DEFINED picotool_DIR)
        set(picotool_DIR ${KB_TOOLS}/picotool/picotool)
    endif()
    if (NOT DEFINED pioasm_DIR)
        set(pioasm_DIR ${KB_TOOLS}/sdktools/pioasm)
    endif()
endif()

include(pico_sdk_import.cmake)

project(kbswitch C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(kbswitch
    src/main.c
    src/kb_host.c
    src/keymap.c
    src/out_usb_horipad.c
    src/usb_descriptors.c
)

target_include_directories(kbswitch PRIVATE src)

target_link_libraries(kbswitch PRIVATE
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_none
    tinyusb_device
    tinyusb_host
    tinyusb_pico_pio_usb
)

pico_enable_stdio_uart(kbswitch 1)
pico_enable_stdio_usb(kbswitch 0)

pico_add_extra_outputs(kbswitch)
