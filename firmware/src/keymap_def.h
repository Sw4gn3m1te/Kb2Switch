// Only for files in firmware/keymaps/
#ifndef KEYMAP_DEF_H_
#define KEYMAP_DEF_H_

#include "class/hid/hid.h"
#include "keymap.h"

#ifndef KEYMAP_ID
#error "KEYMAP_ID is set by CMake for files in firmware/keymaps/"
#endif

// Key names are HID_KEY_* without prefix (W, SPACE, SHIFT_LEFT, ARROW_UP, ...), see class/hid/hid.h
#define BTN(key, btn)    {HID_KEY_##key, ACT_BUTTON, PAD_BTN_##btn}
#define DPAD(key, dir)   {HID_KEY_##key, ACT_DPAD, PAD_DIR_##dir}
#define LSTICK(key, dir) {HID_KEY_##key, ACT_LSTICK, PAD_DIR_##dir}
#define RSTICK(key, dir) {HID_KEY_##key, ACT_RSTICK, PAD_DIR_##dir}
#define HALF_TILT(key)   {HID_KEY_##key, ACT_HALF_TILT, 0}
#define RSTICK_FOLLOW(key) {HID_KEY_##key, ACT_RSTICK_FOLLOW, 0}

// Fixed-size table so the count needs no trailer macro; zeroed tail slots terminate it
#define KEYMAP(display_name)                                         \
    static const keymap_entry_t keymap_entries[KEYMAP_MAX_ENTRIES]; \
    const keymap_t KEYMAP_ID = {display_name, keymap_entries};      \
    static const keymap_entry_t keymap_entries[KEYMAP_MAX_ENTRIES] =

#endif
