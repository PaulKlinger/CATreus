#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H


#include <stdint.h>
#include "config.h"

enum key_layer {
    LAYER_0 = 0,
    LAYER_2 = 2,
    LAYER_3 = 3,
    LAYER_5 = 5,
};

struct encoded_keys encode_keys(struct pressed_keys keys, enum key_layer layer);
bool eq_pressed_keys(struct pressed_keys a, struct pressed_keys b);
enum key_layer get_active_layer(struct pressed_keys keys);

extern bool ctrl_cmd_swapped;
void swap_ctrl_cmd();

#endif // KEY_LAYOUT_H