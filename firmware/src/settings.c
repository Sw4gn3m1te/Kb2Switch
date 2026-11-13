#include "settings.h"

#include <stdio.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico/flash.h"

#define SETTINGS_MAGIC 0x3153424Bu // "KBS1"
// Below the RP2350-E10 reserved last sector and the 2 sectors pico_btstack uses for pairing data
#define SETTINGS_OFFSET (PICO_FLASH_SIZE_BYTES - 4u * FLASH_SECTOR_SIZE)

typedef struct {
    uint32_t magic;
    uint8_t keymap;
} settings_t;

static const settings_t *stored(void) {
    return (const settings_t *)(XIP_BASE + SETTINGS_OFFSET);
}

uint8_t settings_load_keymap(void) {
    const settings_t *s = stored();
    return s->magic == SETTINGS_MAGIC ? s->keymap : 0;
}

static void write_page(void *page) {
    flash_range_erase(SETTINGS_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_OFFSET, page, FLASH_PAGE_SIZE);
}

void settings_save_keymap(uint8_t idx) {
    if (stored()->magic == SETTINGS_MAGIC && stored()->keymap == idx) return;

    static uint8_t page[FLASH_PAGE_SIZE];
    settings_t s = {.magic = SETTINGS_MAGIC, .keymap = idx};
    memset(page, 0xFF, sizeof page);
    memcpy(page, &s, sizeof s);

    int rc = flash_safe_execute(write_page, page, 100);
    if (rc != PICO_OK) printf("settings: save failed %d\n", rc);
}
