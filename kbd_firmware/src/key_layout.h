#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H


#include <stdint.h>
#include "config.h"

struct encoded_keys get_encoded_keys();
bool eq_pressed_keys(struct pressed_keys a, struct pressed_keys b);

extern bool ctrl_cmd_swapped;
void swap_ctrl_cmd();

void init_key_layout();

#endif // KEY_LAYOUT_H