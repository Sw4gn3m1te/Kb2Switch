#ifndef KEYMAP_H_
#define KEYMAP_H_

#include <stdint.h>

#include "kb_host.h"
#include "pad_state.h"

#define KEYMAP_MAX_ENTRIES 128
#define KEYMAP_SLOTS 10 // Ctrl+Shift+1..9,0

typedef enum {
    ACT_BUTTON = 1,
    ACT_DPAD,
    ACT_LSTICK,
    ACT_RSTICK,
    ACT_HALF_TILT,     // left stick at half magnitude
    ACT_RSTICK_FOLLOW, // direction keys drive the right stick at full tilt, left stick neutral
} keymap_action_t;

typedef struct {
    uint8_t usage;
    uint8_t action;
    uint16_t arg;
} keymap_entry_t;

typedef struct {
    const char *name;
    const keymap_entry_t *entries; // KEYMAP_MAX_ENTRIES slots, terminated by usage 0
} keymap_t;

// Loads the saved keymap selection
void keymap_init(void);

// Also handles the Ctrl+Shift+digit keymap hotkey; pad stays neutral while the chord is held
void keymap_apply(const keyset_t *keys, pad_state_t *pad);

#endif
