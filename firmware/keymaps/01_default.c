#include "keymap_def.h"

KEYMAP("Default") {
    LSTICK(W, UP),
    LSTICK(A, LEFT),
    LSTICK(S, DOWN),
    LSTICK(D, RIGHT),

    RSTICK(I, UP),
    RSTICK(J, LEFT),
    RSTICK(K, DOWN),
    RSTICK(L, RIGHT),

    DPAD(ARROW_UP, UP),
    DPAD(ARROW_DOWN, DOWN),
    DPAD(ARROW_LEFT, LEFT),
    DPAD(ARROW_RIGHT, RIGHT),

    BTN(SPACE, A),
    BTN(SHIFT_LEFT, B),
    BTN(E, X),
    BTN(Q, Y),
    BTN(R, L),
    BTN(F, ZL),
    BTN(U, R),
    BTN(O, ZR),
    BTN(ENTER, PLUS),
    BTN(BACKSPACE, MINUS),
    BTN(C, LSTICK),
    BTN(V, RSTICK),
    BTN(H, HOME),
    BTN(P, CAPTURE),

    HALF_TILT(CONTROL_LEFT),
};
