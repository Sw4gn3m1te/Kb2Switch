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

# Each keymaps/*.c becomes one selectable keymap; sorted filename order = Ctrl+Shift+1..9,0
file(GLOB KEYMAP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/keymaps/*.c)
list(SORT KEYMAP_SOURCES)
list(LENGTH KEYMAP_SOURCES KEYMAP_COUNT)
if (KEYMAP_COUNT EQUAL 0 OR KEYMAP_COUNT GREATER 10)
    message(FATAL_ERROR "firmware/keymaps/ needs 1..10 keymap files, found ${KEYMAP_COUNT}")
endif()
set(KEYMAP_DECLS "")
set(KEYMAP_LIST "")
foreach (src ${KEYMAP_SOURCES})
    get_filename_component(stem ${src} NAME_WE)
    string(MAKE_C_IDENTIFIER "keymap_${stem}" id)
    set_source_files_properties(${src} PROPERTIES
        COMPILE_DEFINITIONS KEYMAP_ID=${id}
        COMPILE_OPTIONS "-Wall;-Werror")
    string(APPEND KEYMAP_DECLS "extern const keymap_t ${id};\n")
    string(APPEND KEYMAP_LIST "    &${id},\n")
endforeach()
configure_file(src/keymaps_registry.c.in ${CMAKE_CURRENT_BINARY_DIR}/keymaps_registry.c @ONLY)

add_executable(kbswitch
    src/main.c
    src/kb_host.c
    src/keymap.c
    src/settings.c
    src/out_usb_horipad.c
    src/usb_descriptors.c
    ${KEYMAP_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/keymaps_registry.c
)

target_include_directories(kbswitch PRIVATE src)

target_link_libraries(kbswitch PRIVATE
    pico_stdlib
    pico_multicore
    pico_flash
    hardware_flash
    pico_cyw43_arch_none
    tinyusb_device
    tinyusb_host
    tinyusb_pico_pio_usb
)

pico_enable_stdio_uart(kbswitch 1)
pico_enable_stdio_usb(kbswitch 0)

pico_add_extra_outputs(kbswitch)
