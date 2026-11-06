#ifndef PAD_STATE_H_
#define PAD_STATE_H_

#include <stdint.h>

enum {
    PAD_BTN_A       = 1u << 0,
    PAD_BTN_B       = 1u << 1,
    PAD_BTN_X       = 1u << 2,
    PAD_BTN_Y       = 1u << 3,
    PAD_BTN_L       = 1u << 4,
    PAD_BTN_R       = 1u << 5,
    PAD_BTN_ZL      = 1u << 6,
    PAD_BTN_ZR      = 1u << 7,
    PAD_BTN_MINUS   = 1u << 8,
    PAD_BTN_PLUS    = 1u << 9,
    PAD_BTN_LSTICK  = 1u << 10,
    PAD_BTN_RSTICK  = 1u << 11,
    PAD_BTN_HOME    = 1u << 12,
    PAD_BTN_CAPTURE = 1u << 13,
};

enum {
    PAD_DIR_UP    = 1u << 0,
    PAD_DIR_DOWN  = 1u << 1,
    PAD_DIR_LEFT  = 1u << 2,
    PAD_DIR_RIGHT = 1u << 3,
};

// Output-format independent controller state. Sticks -127..127, +y = up.
typedef struct {
    uint16_t buttons;
    uint8_t dpad;
    int8_t lx, ly;
    int8_t rx, ry;
} pad_state_t;

#endif
