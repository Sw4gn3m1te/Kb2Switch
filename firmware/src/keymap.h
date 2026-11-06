#ifndef KEYMAP_H_
#define KEYMAP_H_

#include "kb_host.h"
#include "pad_state.h"

void keymap_apply(const keyset_t *keys, pad_state_t *pad);

#endif
