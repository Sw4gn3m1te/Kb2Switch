#include <stdio.h>

#include "hardware/clocks.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "kb_host.h"
#include "keymap.h"
#include "out_usb_horipad.h"

#define DEV_RHPORT 0
#define LED_BLINK_MS 500

// LED: off = no keyboard, blink = keyboard but no Switch, on = keyboard + Switch
static void led_task(void) {
    static bool state;
    bool want;
    if (kb_host_count() == 0) {
        want = false;
    } else if (tud_mounted()) {
        want = true;
    } else {
        want = (to_ms_since_boot(get_absolute_time()) / LED_BLINK_MS) & 1;
    }
    if (want != state) {
        state = want;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
    }
}

int main(void) {
    // PIO-USB bit timing needs a multiple of 12 MHz; 120 MHz also stays below RP2350 default clock
    set_sys_clock_khz(120000, true);
    stdio_init_all();
    printf("kbswitch start\n");
    keymap_init();

    // Must run before PIO-USB init so CYW43 takes PIO0 + low DMA channels (see kb_host.c)
    if (cyw43_arch_init()) printf("cyw43 init failed\n");

    kb_host_start();

    tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO};
    tusb_init(DEV_RHPORT, &dev_init);

    keyset_t keys;
    pad_state_t pad;
    while (true) {
        tud_task();
        kb_host_get(&keys);
        keymap_apply(&keys, &pad);
        out_usb_horipad_task(&pad);
        led_task();
    }
}
