#include "key_layout.h"

#include <stdint.h>
#include <zephyr/sys/printk.h>

#include "config.h"
#include "key_matrix.h"
#include "nvs.h"
#include "usb_hid_keys.h"

enum key_layer {
  LAYER_0 = 0,
  LAYER_2 = 2,
  LAYER_3 = 3,
  LAYER_5 = 5,
};

// key with active layer at the time it was pressed

/*
   _______________________________________________________________________________
 | Why do we need to keep state here? | | We can't determine the current state
 of the keys just from the pressed keys, | | because we want to keep the layer
 state at the time each key was pressed.    | | E.g. when typing "a_b", if the a
 is held down for a fraction of a second     | | too long, overlapping with
 shifting to layer 3, then we'd type "a{_b",       | | because to the host {
 looks like a new keypress, so the repeating delay      | | doesn't apply. | | |
 | To prevent this we keep track of the layer at the time each key was pressed,
 | | so we'd still send "a" to the host even after the layer changed to 3. |
 |______________________________________________________________________________|
 */

struct key_with_layer {
  struct key_coord coord;
  enum key_layer layer;
};

// state of currently active keys
struct active_keys_state {
  struct key_with_layer keys[MAX_N_PRESSED_KEYS];
  uint8_t n_active;  // number of active keys
};

struct active_keys_state current_active_keys = {0};

struct key_with_mod {
  uint8_t key;  // HID key code
  uint8_t mod;  // Modifier key mask (e.g., KEY_MOD_LCTRL)
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
        {KEY_LEFTALT, KEY_MOD_LALT},
        {KEY_GRAVE, 0},
        {KEY_ENTER, 0},
    },
};

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
    },
};

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
        {KEY_SLASH, KEY_MOD_LSHIFT},  // ?
        {KEY_9, KEY_MOD_LSHIFT},      // (
        {KEY_0, KEY_MOD_LSHIFT},      // )
        {KEY_MINUS, 0},
        {KEY_SEMICOLON, KEY_MOD_LSHIFT},  // :
    },
    {
        {0, 0},
        {0, 0},
        {KEY_4, KEY_MOD_LSHIFT},          // $
        {KEY_BACKSLASH, KEY_MOD_LSHIFT},  // |
        {KEY_GRAVE, KEY_MOD_LSHIFT},      // ~
        {KEY_GRAVE, 0},
        {KEY_KPPLUS, 0},                   // +
        {KEY_5, KEY_MOD_LSHIFT},           // %
        {KEY_APOSTROPHE, KEY_MOD_LSHIFT},  // "
        {KEY_APOSTROPHE, 0},
        {KEY_SEMICOLON, 0},
    },
    {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}},
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
    {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}},
};

enum key_layer get_active_layer(struct pressed_keys keys) {
  if (keys.n_pressed == 0) {
    return LAYER_0;  // Default layer
  }

  // Check for specific keys to determine the active layer
  for (uint8_t i = 0; i < keys.n_pressed; i++) {
    if (keys.keys[i].row == layer_2_key.row &&
        keys.keys[i].col == layer_2_key.col) {
      return LAYER_2;
    } else if (keys.keys[i].row == layer_3_key.row &&
               keys.keys[i].col == layer_3_key.col) {
      return LAYER_3;
    } else if (keys.keys[i].row == layer_5_key.row &&
               keys.keys[i].col == layer_5_key.col) {
      return LAYER_5;
    }
  }

  return LAYER_0;  // Default to layer 0 if no special keys are pressed
}

struct key_with_mod get_key_with_mod(struct key_with_layer key) {
  struct key_with_mod res = {0, 0};
  switch (key.layer) {
    case LAYER_0:
      return key_map_layer_0[key.coord.row][key.coord.col];
    case LAYER_2:
      res = key_map_layer_2[key.coord.row][key.coord.col];
      break;
    case LAYER_3:
      res = key_map_layer_3[key.coord.row][key.coord.col];
      break;
    case LAYER_5:
      res = key_map_layer_5[key.coord.row][key.coord.col];
      break;
  }
  if (res.key == 0) {
    return key_map_layer_0[key.coord.row][key.coord.col];  // Fallback to layer
                                                           // 0 if no key found
  }
  return res;
}

void update_active_keys_state() {
  // update active keys state based on currently pressed keys

  struct pressed_keys keys = current_pressed_keys;
  enum key_layer current_layer = get_active_layer(keys);

  struct active_keys_state new_active_keys = {0};
  // keep all the keys that are still pressed
  for (uint8_t i = 0; i < current_active_keys.n_active; i++) {
    bool still_pressed = false;
    for (uint8_t j = 0; j < keys.n_pressed; j++) {
      if (keq(current_active_keys.keys[i].coord, keys.keys[j])) {
        still_pressed = true;
        break;
      }
    }
    if (still_pressed) {
      new_active_keys.keys[new_active_keys.n_active] =
          current_active_keys.keys[i];
      new_active_keys.n_active++;
    }
  }

  // add new pressed keys
  for (uint8_t i = 0; i < keys.n_pressed; i++) {
    bool new_pressed = true;
    for (uint8_t j = 0; j < new_active_keys.n_active; j++) {
      if (keq(new_active_keys.keys[j].coord, keys.keys[i])) {
        new_pressed = false;
        break;
      }
    }
    if (new_pressed && new_active_keys.n_active < MAX_N_PRESSED_KEYS) {
      // Add new key to active keys state
      new_active_keys.keys[new_active_keys.n_active].coord = keys.keys[i];
      new_active_keys.keys[new_active_keys.n_active].layer = current_layer;
      new_active_keys.n_active++;
    }
  }
  current_active_keys = new_active_keys;
}

struct encoded_keys get_encoded_keys() {
  update_active_keys_state();

  struct encoded_keys encoded = {0};
  uint8_t n_keys = 0;
  for (uint8_t i = 0; i < current_active_keys.n_active; i++) {
    struct key_with_mod k_mod = get_key_with_mod(current_active_keys.keys[i]);
    if (k_mod.key != 0) {
      if (n_keys < MAX_N_ENCODED_KEYS) {
        encoded.keys[n_keys] = k_mod.key;
        encoded.modifier_mask |= k_mod.mod;
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

bool ctrl_cmd_swapped = false;

void swap_ctrl_cmd_in_keymap() {
  // Swap Ctrl and Cmd keys in the key map
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 11; col++) {
      if (key_map_layer_0[row][col].key == KEY_LEFTCTRL) {
        key_map_layer_0[row][col].key = KEY_LEFTMETA;
        key_map_layer_0[row][col].mod = KEY_MOD_LMETA;
      } else if (key_map_layer_0[row][col].key == KEY_LEFTMETA) {
        key_map_layer_0[row][col].key = KEY_LEFTCTRL;
        key_map_layer_0[row][col].mod = KEY_MOD_LCTRL;
      }
    }
  }
}

void swap_ctrl_cmd() {
  ctrl_cmd_swapped = !ctrl_cmd_swapped;
  swap_ctrl_cmd_in_keymap();
  nvs_store_ctrl_cmd(ctrl_cmd_swapped);
}

void init_key_layout() {
  if (nvs_get_ctrl_cmd_config() != ctrl_cmd_swapped) {
    ctrl_cmd_swapped = !ctrl_cmd_swapped;
    swap_ctrl_cmd_in_keymap();
  }
}
