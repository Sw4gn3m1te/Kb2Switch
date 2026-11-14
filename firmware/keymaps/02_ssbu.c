#include "keymap_def.h"

KEYMAP("SSBU") {
    LSTICK(W, UP),
    LSTICK(A, LEFT),
    LSTICK(S, DOWN),
    LSTICK(D, RIGHT),

    // Same key twice: A press with half-tilted left stick = tilt attack
    BTN(J, A),
    HALF_TILT(J),
    HALF_TILT(EUROPE_2), // NUBS, DE layout: <

    BTN(K, B),
    BTN(SPACE, X),
    BTN(CAPS_LOCK, X),
    BTN(SHIFT_LEFT, ZL),
    BTN(SHIFT_RIGHT, ZL),
    BTN(SEMICOLON, ZL), // DE layout: ö

    RSTICK_FOLLOW(L),

    BTN(BACKSPACE, PLUS),
};
