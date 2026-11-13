#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

// Persistent settings in flash. Saving pauses core 1 (~50 ms), so only call on user action
uint8_t settings_load_keymap(void);
void settings_save_keymap(uint8_t idx);

#endif
