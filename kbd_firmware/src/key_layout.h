#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H


#include <stdint.h>
#include "config.h"

struct encoded_keys encode_keys(struct pressed_keys keys);
bool eq_pressed_keys(struct pressed_keys a, struct pressed_keys b);

#endif // KEY_LAYOUT_H