#include <stdint.h>
#include "key_layout.h"
#include "usb_hid_keys.h"

#include "key_matrix.h"

enum key_layer {
    LAYER_0 = 0,
    LAYER_2 = 2,
    LAYER_3 = 3,
    LAYER_5 = 5,
};

struct key_with_mod
{
    uint8_t key; // HID key code
    uint8_t mod; // Modifier key mask (e.g., KEY_MOD_LCTRL)
};

struct key_coord layer_2_key = {3, 7};
struct key_coord layer_3_key = {3, 6};
struct key_coord layer_5_key = {3, 1};

struct key_with_mod key_map_layer_0[4][11] = {
    {
        {KEY_X, 0},
        {KEY_V, 0},
        {KEY_L, 0},
        {KEY_C, 0},
        {KEY_W, 0},
        {KEY_Z, 0},
        {KEY_K, 0},
        {KEY_H, 0},
        {KEY_G, 0},
        {KEY_F, 0},
        {KEY_Q, 0},
    },
    {
        {KEY_U, 0},
        {KEY_I, 0},
        {KEY_A, 0},
        {KEY_E, 0},
        {KEY_O, 0},
        {KEY_LEFTCTRL, KEY_MOD_LCTRL},
        {KEY_S, 0},
        {KEY_N, 0},
        {KEY_R, 0},
        {KEY_T, 0},
        {KEY_D, 0},
    },
    {
        {KEY_LEFTSHIFT, KEY_MOD_LSHIFT},
        {KEY_3, KEY_MOD_LSHIFT},
        {KEY_Y, 0},
        {KEY_P, 0},
        {KEY_TAB, 0},
        {0, 0},
        {KEY_B, 0},
        {KEY_M, 0},
        {KEY_COMMA, 0},
        {KEY_DOT, 0},
        {KEY_J, 0},
    },
    {
        {KEY_ESC, 0},
        {KEY_FN, 0},
        {0, 0},
        {KEY_BACKSPACE, 0},
        {KEY_SPACE, 0},
        {KEY_LEFTMETA, KEY_MOD_LMETA},
        {0, 0},
        {0, 0},
        {KEY_LEFTALT, 0},
        {KEY_GRAVE, 0},
        {KEY_ENTER, 0},
    }};

struct key_with_mod key_map_layer_2[4][11] = {
    {
        {KEY_PAGEUP, 0},
        {KEY_BACKSPACE, 0},
        {KEY_UP, 0},
        {KEY_DELETE, 0},
        {KEY_PAGEDOWN, 0},
        {0, 0},
        {0, 0},
        {KEY_7, 0},
        {KEY_8, 0},
        {KEY_9, 0},
        {0, 0},
    },
    {
        {KEY_HOME, 0},
        {KEY_LEFT, 0},
        {KEY_DOWN, 0},
        {KEY_RIGHT, 0},
        {KEY_END, 0},
        {0, 0},
        {0, 0},
        {KEY_4, 0},
        {KEY_5, 0},
        {KEY_6, 0},
        {0, 0},
    },
    {
        {0, 0},
        {0, 0},
        {KEY_3, KEY_MOD_LSHIFT},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {KEY_1, 0},
        {KEY_2, 0},
        {KEY_3, 0},
        {0, 0},
    },
    {
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {KEY_0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
    }};

struct key_with_mod key_map_layer_3[4][11] = {
    {
        {KEY_2, KEY_MOD_LSHIFT},
        {KEY_MINUS, KEY_MOD_LSHIFT},
        {KEY_LEFTBRACE, 0},
        {KEY_RIGHTBRACE, 0},
        {KEY_6, KEY_MOD_LSHIFT},
        {0, 0},
        {KEY_1, KEY_MOD_LSHIFT},
        {KEY_COMMA, KEY_MOD_LSHIFT},
        {KEY_DOT, KEY_MOD_LSHIFT},
        {KEY_EQUAL, 0},
        {KEY_7, KEY_MOD_LSHIFT},
    },
    {
        {KEY_BACKSLASH, 0},
        {KEY_SLASH, 0},
        {KEY_LEFTBRACE, KEY_MOD_LSHIFT},
        {KEY_RIGHTBRACE, KEY_MOD_RSHIFT},
        {KEY_8, KEY_MOD_LSHIFT},
        {0},
        {KEY_SLASH, KEY_MOD_LSHIFT}, // ?
        {KEY_9, KEY_MOD_LSHIFT},     // (
        {KEY_0, KEY_MOD_LSHIFT},     // )
        {KEY_MINUS, 0},
        {KEY_SEMICOLON, KEY_MOD_LSHIFT}, // :
    },
    {
        {0, 0},
        {KEY_3, 0},
        {KEY_4, KEY_MOD_LSHIFT},         // $
        {KEY_BACKSLASH, KEY_MOD_LSHIFT}, // |
        {KEY_GRAVE, KEY_MOD_LSHIFT},     //
        {KEY_GRAVE, 0},
        {KEY_EQUAL, KEY_MOD_LSHIFT},      // +
        {KEY_5, KEY_MOD_LSHIFT},          // %
        {KEY_APOSTROPHE, KEY_MOD_LSHIFT}, // "
        {KEY_APOSTROPHE, 0},
        {KEY_SEMICOLON, 0},
    },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

struct key_with_mod key_map_layer_5[4][11] = {
    {
        {KEY_F1, 0},
        {KEY_F2, 0},
        {KEY_F3, 0},
        {KEY_F4, 0},
        {KEY_F5, 0},
        {0, 0},
        {KEY_F6, 0},
        {KEY_F7, 0},
        {KEY_F8, 0},
        {KEY_F9, 0},
        {KEY_F10, 0},
    },
    {
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {KEY_F11, 0},
    },
    {
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {0, 0},
        {KEY_F12, 0},
    },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


enum key_layer get_active_layer(struct pressed_keys keys) {
    if (keys.n_pressed == 0) {
        return LAYER_0; // Default layer
    }
    
    // Check for specific keys to determine the active layer
    for (uint8_t i = 0; i < keys.n_pressed; i++) {
        if (keys.keys[i].row == layer_2_key.row && keys.keys[i].col == layer_2_key.col) {
            return LAYER_2;
        } else if (keys.keys[i].row == layer_3_key.row && keys.keys[i].col == layer_3_key.col) {
            return LAYER_3;
        } else if (keys.keys[i].row == layer_5_key.row && keys.keys[i].col == layer_5_key.col) {
            return LAYER_5;
        }
    }
    
    return LAYER_0; // Default to layer 0 if no special keys are pressed
}

struct key_with_mod get_key_with_mod(struct key_coord coord, enum key_layer layer) {
    struct key_with_mod res = {0, 0};
    switch (layer) {
        case LAYER_0:
            return key_map_layer_0[coord.row][coord.col];
        case LAYER_2:
            res = key_map_layer_2[coord.row][coord.col];
            break;
        case LAYER_3:
            res = key_map_layer_3[coord.row][coord.col];
            break;
        case LAYER_5:
            res = key_map_layer_5[coord.row][coord.col];
            break;
    }
    if (res.key == 0) {
        return key_map_layer_0[coord.row][coord.col]; // Fallback to layer 0 if no key found
    }
    return res;
}

struct encoded_keys encode_keys(struct pressed_keys keys) {
    uint8_t layer = get_active_layer(keys);
    printk("Active layer: %d\n", layer);
    struct encoded_keys encoded = {0};
    uint8_t n_keys = 0;
    for (uint8_t i = 0; i < keys.n_pressed; i++) {
        struct key_with_mod key = get_key_with_mod(keys.keys[i], layer);
        if (key.key != 0) {
            if (n_keys < MAX_N_ENCODED_KEYS) {
                encoded.keys[n_keys] = key.key;
                encoded.modifier_mask |= key.mod;
                n_keys++;
            } else {
                // Set all keys to KEY_ERR_OVF if overflow occurs
                for (uint8_t j = 0; j < MAX_N_ENCODED_KEYS; j++) {
                    encoded.keys[j] = KEY_ERR_OVF;
                }
                break;
            }
        }
    }
    return encoded;
}