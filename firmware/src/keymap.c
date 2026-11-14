#include "keymap.h"

#include <stdio.h>

#include "class/hid/hid.h"
#include "settings.h"

#define STICK_FULL 127
#define STICK_HALF 64

extern const keymap_t *const keymaps[];
extern const uint8_t keymap_count;

static uint8_t active;

static void log_active(void) {
    printf("keymap: %u %s\n", active + 1, keymaps[active]->name);
}

void keymap_init(void) {
    uint8_t idx = settings_load_keymap();
    active = idx < keymap_count ? idx : 0;
    log_active();
}

static void select_keymap(uint8_t idx) {
    if (idx >= keymap_count || idx == active) return;
    active = idx;
    log_active();
    settings_save_keymap(idx);
}

// Neutral only while a digit is part of the chord, so Ctrl/Shift stay usable as game keys
static bool handle_hotkey(const keyset_t *keys) {
    static uint16_t prev_digits;
    uint16_t digits = 0;
    for (uint8_t i = 0; i < KEYMAP_SLOTS; i++) {
        if (keyset_has(keys, HID_KEY_1 + i)) digits |= 1u << i; // HID_KEY_1..9 then HID_KEY_0
    }
    uint16_t pressed = digits & ~prev_digits;
    prev_digits = digits;

    bool ctrl = keyset_has(keys, HID_KEY_CONTROL_LEFT) || keyset_has(keys, HID_KEY_CONTROL_RIGHT);
    bool shift = keyset_has(keys, HID_KEY_SHIFT_LEFT) || keyset_has(keys, HID_KEY_SHIFT_RIGHT);
    if (!(ctrl && shift && digits)) return false;
    for (uint8_t i = 0; i < KEYMAP_SLOTS; i++) {
        if (pressed & (1u << i)) select_keymap(i);
    }
    return true;
}

static uint8_t cancel_opposites(uint8_t dir) {
    if ((dir & PAD_DIR_UP) && (dir & PAD_DIR_DOWN)) dir &= (uint8_t)~(PAD_DIR_UP | PAD_DIR_DOWN);
    if ((dir & PAD_DIR_LEFT) && (dir & PAD_DIR_RIGHT)) dir &= (uint8_t)~(PAD_DIR_LEFT | PAD_DIR_RIGHT);
    return dir;
}

static void dir_to_stick(uint8_t dir, bool half, int8_t *x, int8_t *y) {
    int dx = ((dir & PAD_DIR_RIGHT) ? 1 : 0) - ((dir & PAD_DIR_LEFT) ? 1 : 0);
    int dy = ((dir & PAD_DIR_UP) ? 1 : 0) - ((dir & PAD_DIR_DOWN) ? 1 : 0);
    int mag = half ? STICK_HALF : STICK_FULL;
    if (dx && dy) mag = mag * 181 / 256; // 1/sqrt(2)
    *x = (int8_t)(dx * mag);
    *y = (int8_t)(dy * mag);
}

void keymap_apply(const keyset_t *keys, pad_state_t *pad) {
    if (handle_hotkey(keys)) {
        *pad = (pad_state_t){0};
        return;
    }

    uint16_t buttons = 0;
    uint8_t dpad = 0, ldir = 0, rdir = 0;
    bool half = false, follow = false;

    const keymap_entry_t *e = keymaps[active]->entries;
    for (const keymap_entry_t *end = e + KEYMAP_MAX_ENTRIES; e < end && e->usage; e++) {
        if (!keyset_has(keys, e->usage)) continue;
        switch (e->action) {
            case ACT_BUTTON: buttons |= e->arg; break;
            case ACT_DPAD: dpad |= (uint8_t)e->arg; break;
            case ACT_LSTICK: ldir |= (uint8_t)e->arg; break;
            case ACT_RSTICK: rdir |= (uint8_t)e->arg; break;
            case ACT_HALF_TILT: half = true; break;
            case ACT_RSTICK_FOLLOW: follow = true; break;
        }
    }
    // Left stick released while following: a held run direction plus C-stick would read as a dash attack in SSBU
    if (follow) {
        rdir |= ldir;
        ldir = 0;
    }

    pad->buttons = buttons;
    pad->dpad = cancel_opposites(dpad);
    dir_to_stick(cancel_opposites(ldir), half, &pad->lx, &pad->ly);
    dir_to_stick(cancel_opposites(rdir), false, &pad->rx, &pad->ry);
}
