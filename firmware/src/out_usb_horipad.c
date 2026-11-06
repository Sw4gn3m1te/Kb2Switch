#include "out_usb_horipad.h"

#include <string.h>

#include "pico/time.h"
#include "tusb.h"

#define RESEND_MS 8

enum {
    HP_Y = 1u << 0,
    HP_B = 1u << 1,
    HP_A = 1u << 2,
    HP_X = 1u << 3,
    HP_L = 1u << 4,
    HP_R = 1u << 5,
    HP_ZL = 1u << 6,
    HP_ZR = 1u << 7,
    HP_MINUS = 1u << 8,
    HP_PLUS = 1u << 9,
    HP_LSTICK = 1u << 10,
    HP_RSTICK = 1u << 11,
    HP_HOME = 1u << 12,
    HP_CAPTURE = 1u << 13,
};

#define HP_HAT_NEUTRAL 8

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t hat;
    uint8_t lx, ly, rx, ry;
    uint8_t vendor;
} horipad_report_t;

static const struct {
    uint16_t pad;
    uint16_t hp;
} button_map[] = {
    {PAD_BTN_A, HP_A},           {PAD_BTN_B, HP_B},           {PAD_BTN_X, HP_X},
    {PAD_BTN_Y, HP_Y},           {PAD_BTN_L, HP_L},           {PAD_BTN_R, HP_R},
    {PAD_BTN_ZL, HP_ZL},         {PAD_BTN_ZR, HP_ZR},         {PAD_BTN_MINUS, HP_MINUS},
    {PAD_BTN_PLUS, HP_PLUS},     {PAD_BTN_LSTICK, HP_LSTICK}, {PAD_BTN_RSTICK, HP_RSTICK},
    {PAD_BTN_HOME, HP_HOME},     {PAD_BTN_CAPTURE, HP_CAPTURE},
};

// Indexed by PAD_DIR_* mask (opposites already cancelled)
static const uint8_t hat_map[16] = {
    [0] = HP_HAT_NEUTRAL,
    [PAD_DIR_UP] = 0,
    [PAD_DIR_UP | PAD_DIR_RIGHT] = 1,
    [PAD_DIR_RIGHT] = 2,
    [PAD_DIR_DOWN | PAD_DIR_RIGHT] = 3,
    [PAD_DIR_DOWN] = 4,
    [PAD_DIR_DOWN | PAD_DIR_LEFT] = 5,
    [PAD_DIR_LEFT] = 6,
    [PAD_DIR_UP | PAD_DIR_LEFT] = 7,
    [PAD_DIR_UP | PAD_DIR_DOWN] = HP_HAT_NEUTRAL,
    [PAD_DIR_LEFT | PAD_DIR_RIGHT] = HP_HAT_NEUTRAL,
    [PAD_DIR_UP | PAD_DIR_LEFT | PAD_DIR_RIGHT] = HP_HAT_NEUTRAL,
    [PAD_DIR_DOWN | PAD_DIR_LEFT | PAD_DIR_RIGHT] = HP_HAT_NEUTRAL,
    [PAD_DIR_UP | PAD_DIR_DOWN | PAD_DIR_LEFT] = HP_HAT_NEUTRAL,
    [PAD_DIR_UP | PAD_DIR_DOWN | PAD_DIR_RIGHT] = HP_HAT_NEUTRAL,
    [PAD_DIR_UP | PAD_DIR_DOWN | PAD_DIR_LEFT | PAD_DIR_RIGHT] = HP_HAT_NEUTRAL,
};

static horipad_report_t last;
static absolute_time_t last_sent;

static void build(const pad_state_t *pad, horipad_report_t *r) {
    r->buttons = 0;
    for (size_t i = 0; i < sizeof button_map / sizeof button_map[0]; i++) {
        if (pad->buttons & button_map[i].pad) r->buttons |= button_map[i].hp;
    }
    r->hat = hat_map[pad->dpad & 0x0F];
    // HID Y axis grows downward
    r->lx = (uint8_t)(128 + pad->lx);
    r->ly = (uint8_t)(128 - pad->ly);
    r->rx = (uint8_t)(128 + pad->rx);
    r->ry = (uint8_t)(128 - pad->ry);
    r->vendor = 0;
}

void out_usb_horipad_task(const pad_state_t *pad) {
    if (!tud_mounted() || !tud_hid_ready()) return;

    horipad_report_t r;
    build(pad, &r);

    bool changed = memcmp(&r, &last, sizeof r) != 0;
    if (!changed && absolute_time_diff_us(last_sent, get_absolute_time()) < RESEND_MS * 1000) return;

    if (tud_hid_report(0, &r, sizeof r)) {
        last = r;
        last_sent = get_absolute_time();
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    uint16_t n = sizeof last < reqlen ? sizeof last : reqlen;
    memcpy(buffer, &last, n);
    return n;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
