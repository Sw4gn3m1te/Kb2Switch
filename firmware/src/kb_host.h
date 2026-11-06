#ifndef KB_HOST_H_
#define KB_HOST_H_

#include <stdbool.h>
#include <stdint.h>

#define KB_PIN_DP 2 // D- must be KB_PIN_DP + 1

// Bitmap of pressed HID keyboard usages (0x00-0xFF), modifiers as 0xE0-0xE7
typedef struct {
    uint32_t bits[8];
} keyset_t;

static inline bool keyset_has(const keyset_t *k, uint8_t usage) {
    return (k->bits[usage >> 5] >> (usage & 31)) & 1u;
}

// Launches the keyboard host on core 1
void kb_host_start(void);

// Thread-safe snapshot of pressed keys (union over all keyboards)
void kb_host_get(keyset_t *out);

uint8_t kb_host_count(void);

#endif
