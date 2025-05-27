#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_N_ENCODED_KEYS 6
#define MAX_N_PRESSED_KEYS 6
#define DEEP_SLEEP_TIMEOUT_S 60 * 30
#define DEEP_SLEEP_ADVERTISING_TIMEOUT_S 60 * 5

struct key_coord {
    uint8_t row;
    uint8_t col;
};

inline bool keq(struct key_coord a, struct key_coord b) {
    return a.row == b.row && a.col == b.col;
}

// physical keys pressed
struct pressed_keys {
    bool wake_pressed;
    struct key_coord keys[MAX_N_PRESSED_KEYS];
    uint8_t n_pressed;
};

// keys encoded for USB HID report
struct encoded_keys {
    uint8_t modifier_mask;
    uint8_t keys[MAX_N_ENCODED_KEYS];
};

#endif  // CONFIG_H