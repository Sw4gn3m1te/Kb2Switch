#include "keymap_def.h"

KEYMAP("SSBU") {
    LSTICK(W, UP),
    LSTICK(A, LEFT),
    LSTICK(S, DOWN),
    LSTICK(D, RIGHT),

    // Same key twice: A press with half-tilted left stick = tilt attack
    BTN(J, A),
    HALF_TILT(J),

    BTN(K, B),
    BTN(L, ZR),
    BTN(SPACE, X),
    BTN(SHIFT_LEFT, R),
    BTN(SHIFT_RIGHT, R),

    RSTICK_FOLLOW(CAPS_LOCK),

    BTN(BACKSPACE, PLUS),
};
