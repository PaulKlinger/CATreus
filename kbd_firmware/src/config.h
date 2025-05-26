#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_N_ENCODED_KEYS 6
#define MAX_N_PRESSED_KEYS 6


struct key_coord
{
   uint8_t row;
   uint8_t col;
};

struct pressed_keys
{
   bool wake_pressed;
   struct key_coord keys[MAX_N_PRESSED_KEYS];
   uint8_t n_pressed;
};


struct encoded_keys {
    uint8_t modifier_mask;
    uint8_t keys[MAX_N_ENCODED_KEYS];
};

#endif // CONFIG_H