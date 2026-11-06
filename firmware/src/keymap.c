#include "keymap.h"

#include <stddef.h>

#include "class/hid/hid.h"

#define STICK_FULL 127
#define STICK_HALF 64

typedef enum {
    ACT_BUTTON,
    ACT_DPAD,
    ACT_LSTICK,
    ACT_RSTICK,
    ACT_HALF_TILT,
} action_t;

typedef struct {
    uint8_t usage;
    uint8_t action;
    uint16_t arg;
} keymap_entry_t;

static const keymap_entry_t keymap[] = {
    {HID_KEY_W, ACT_LSTICK, PAD_DIR_UP},
    {HID_KEY_A, ACT_LSTICK, PAD_DIR_LEFT},
    {HID_KEY_S, ACT_LSTICK, PAD_DIR_DOWN},
    {HID_KEY_D, ACT_LSTICK, PAD_DIR_RIGHT},

    {HID_KEY_I, ACT_RSTICK, PAD_DIR_UP},
    {HID_KEY_J, ACT_RSTICK, PAD_DIR_LEFT},
    {HID_KEY_K, ACT_RSTICK, PAD_DIR_DOWN},
    {HID_KEY_L, ACT_RSTICK, PAD_DIR_RIGHT},

    {HID_KEY_ARROW_UP, ACT_DPAD, PAD_DIR_UP},
    {HID_KEY_ARROW_DOWN, ACT_DPAD, PAD_DIR_DOWN},
    {HID_KEY_ARROW_LEFT, ACT_DPAD, PAD_DIR_LEFT},
    {HID_KEY_ARROW_RIGHT, ACT_DPAD, PAD_DIR_RIGHT},

    {HID_KEY_SPACE, ACT_BUTTON, PAD_BTN_A},
    {HID_KEY_SHIFT_LEFT, ACT_BUTTON, PAD_BTN_B},
    {HID_KEY_E, ACT_BUTTON, PAD_BTN_X},
    {HID_KEY_Q, ACT_BUTTON, PAD_BTN_Y},
    {HID_KEY_R, ACT_BUTTON, PAD_BTN_L},
    {HID_KEY_F, ACT_BUTTON, PAD_BTN_ZL},
    {HID_KEY_U, ACT_BUTTON, PAD_BTN_R},
    {HID_KEY_O, ACT_BUTTON, PAD_BTN_ZR},
    {HID_KEY_ENTER, ACT_BUTTON, PAD_BTN_PLUS},
    {HID_KEY_BACKSPACE, ACT_BUTTON, PAD_BTN_MINUS},
    {HID_KEY_C, ACT_BUTTON, PAD_BTN_LSTICK},
    {HID_KEY_V, ACT_BUTTON, PAD_BTN_RSTICK},
    {HID_KEY_H, ACT_BUTTON, PAD_BTN_HOME},
    {HID_KEY_P, ACT_BUTTON, PAD_BTN_CAPTURE},

    {HID_KEY_CONTROL_LEFT, ACT_HALF_TILT, 0},
};

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
    uint16_t buttons = 0;
    uint8_t dpad = 0, ldir = 0, rdir = 0;
    bool half = false;

    for (size_t i = 0; i < sizeof keymap / sizeof keymap[0]; i++) {
        const keymap_entry_t *e = &keymap[i];
        if (!keyset_has(keys, e->usage)) continue;
        switch (e->action) {
            case ACT_BUTTON: buttons |= e->arg; break;
            case ACT_DPAD: dpad |= (uint8_t)e->arg; break;
            case ACT_LSTICK: ldir |= (uint8_t)e->arg; break;
            case ACT_RSTICK: rdir |= (uint8_t)e->arg; break;
            case ACT_HALF_TILT: half = true; break;
        }
    }

    pad->buttons = buttons;
    pad->dpad = cancel_opposites(dpad);
    dir_to_stick(cancel_opposites(ldir), half, &pad->lx, &pad->ly);
    dir_to_stick(cancel_opposites(rdir), half, &pad->rx, &pad->ry);
}
