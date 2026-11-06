#ifndef OUT_USB_HORIPAD_H_
#define OUT_USB_HORIPAD_H_

#include "pad_state.h"

// Call from main loop after tud_task(); sends a report when changed or every 8 ms
void out_usb_horipad_task(const pad_state_t *pad);

#endif
