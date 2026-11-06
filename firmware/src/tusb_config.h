#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_DEBUG          0

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))

// rhport 0: native USB = device (to Switch), rhport 1: PIO-USB = host (keyboard)
#define CFG_TUD_ENABLED         1
#define CFG_TUD_MAX_SPEED       OPT_MODE_FULL_SPEED
#define CFG_TUH_ENABLED         1
#define CFG_TUH_RPI_PIO_USB     1
#define CFG_TUH_MAX_SPEED       OPT_MODE_FULL_SPEED

#define CFG_TUD_ENDPOINT0_SIZE  64
#define CFG_TUD_HID             1
#define CFG_TUD_HID_EP_BUFSIZE  64

#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HUB             1
#define CFG_TUH_DEVICE_MAX      (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_HID             (3 * CFG_TUH_DEVICE_MAX)
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64

#endif
