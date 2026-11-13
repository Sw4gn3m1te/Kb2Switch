#include "kb_host.h"

#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pio_usb.h"
#include "tusb.h"

#define KB_RHPORT 1
#define KB_SLOTS 4

typedef struct {
    bool used;
    uint8_t dev_addr;
    uint8_t idx;
    hid_keyboard_report_t report;
} kb_slot_t;

// Slots are only touched from core 1 (TinyUSB host callbacks)
static kb_slot_t slots[KB_SLOTS];

static spin_lock_t *lock;
static keyset_t shared_keys;
static uint8_t shared_count;

static void keyset_set(keyset_t *k, uint8_t usage) {
    k->bits[usage >> 5] |= 1u << (usage & 31);
}

static void publish(void) {
    keyset_t keys;
    uint8_t count = 0;
    memset(&keys, 0, sizeof keys);

    for (int i = 0; i < KB_SLOTS; i++) {
        if (!slots[i].used) continue;
        count++;
        const hid_keyboard_report_t *r = &slots[i].report;
        for (int m = 0; m < 8; m++) {
            if (r->modifier & (1u << m)) keyset_set(&keys, (uint8_t)(HID_KEY_CONTROL_LEFT + m));
        }
        for (int j = 0; j < 6; j++) {
            if (r->keycode[j] >= HID_KEY_A) keyset_set(&keys, r->keycode[j]);
        }
    }

    uint32_t save = spin_lock_blocking(lock);
    shared_keys = keys;
    shared_count = count;
    spin_unlock(lock, save);
}

static kb_slot_t *find_slot(uint8_t dev_addr, uint8_t idx) {
    for (int i = 0; i < KB_SLOTS; i++) {
        if (slots[i].used && slots[i].dev_addr == dev_addr && slots[i].idx == idx) return &slots[i];
    }
    return NULL;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report_desc, uint16_t desc_len) {
    (void)report_desc;
    (void)desc_len;
    if (tuh_hid_interface_protocol(dev_addr, idx) != HID_ITF_PROTOCOL_KEYBOARD) return;

    for (int i = 0; i < KB_SLOTS; i++) {
        if (slots[i].used) continue;
        memset(&slots[i], 0, sizeof slots[i]);
        slots[i].used = true;
        slots[i].dev_addr = dev_addr;
        slots[i].idx = idx;
        printf("kb: keyboard mounted dev=%u idx=%u\n", dev_addr, idx);
        publish();
        tuh_hid_receive_report(dev_addr, idx);
        return;
    }
    printf("kb: no free slot for dev=%u idx=%u\n", dev_addr, idx);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t idx) {
    kb_slot_t *s = find_slot(dev_addr, idx);
    if (!s) return;
    s->used = false;
    printf("kb: keyboard removed dev=%u idx=%u\n", dev_addr, idx);
    publish();
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report, uint16_t len) {
    kb_slot_t *s = find_slot(dev_addr, idx);
    if (!s) return;

    if (len >= sizeof(hid_keyboard_report_t)) {
        const hid_keyboard_report_t *r = (const hid_keyboard_report_t *)report;
        // Phantom/rollover state (all keycodes = 0x01): keep last valid state
        if (r->keycode[0] != 0x01) {
            s->report = *r;
            publish();
        }
    }
    tuh_hid_receive_report(dev_addr, idx);
}

static void core1_main(void) {
    flash_safe_execute_core_init();

    pio_usb_configuration_t cfg = PIO_USB_DEFAULT_CONFIG;
    cfg.pin_dp = KB_PIN_DP;
    // PIO1 + DMA 11 so the CYW43 driver (claims PIO0 and the lowest free DMA channels) never collides
    cfg.pio_tx_num = 1;
    cfg.pio_rx_num = 1;
    cfg.tx_ch = 11;
    tuh_configure(KB_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &cfg);

    tusb_rhport_init_t host_init = {.role = TUSB_ROLE_HOST, .speed = TUSB_SPEED_AUTO};
    tusb_init(KB_RHPORT, &host_init);

    while (true) {
        tuh_task();
    }
}

void kb_host_start(void) {
    lock = spin_lock_init(spin_lock_claim_unused(true));
    multicore_reset_core1();
    multicore_launch_core1(core1_main);
}

void kb_host_get(keyset_t *out) {
    uint32_t save = spin_lock_blocking(lock);
    *out = shared_keys;
    spin_unlock(lock, save);
}

uint8_t kb_host_count(void) {
    uint32_t save = spin_lock_blocking(lock);
    uint8_t c = shared_count;
    spin_unlock(lock, save);
    return c;
}
