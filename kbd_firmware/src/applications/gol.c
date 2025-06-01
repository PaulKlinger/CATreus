#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "../config.h"
#include "../display.h"
#include "breakout.h"
#include "strings.h"
#include "utils.h"

#define height 32
#define width 64
#define byte_width 8

static bool get_cell_from_buffer(uint8_t x, uint8_t y) {
  return lcd_check_buffer(x * 2, y * 2);
}

static void draw_cell(uint8_t x, uint8_t y) {
  lcd_fillRect(x * 2, y * 2, x * 2 + 1, y * 2 + 1, 1);
}

static void display_board(BitMatrix board) {
  lcd_clear_buffer();
  uint16_t live_cells = 0;
  for (uint8_t y = 0; y < height; y++) {
    for (uint8_t x = 0; x < width; x++) {
      if (bitmatrix_get(board, x, y)) {
        draw_cell(x, y);
        live_cells++;
      }
    };
  };
  lcd_display();
}

static void update_board(BitMatrix board) {
  uint8_t neighbors;

  // TODO: This section is actually performance critical
  // (updates become seriously slow if modulo isn't inlined).
  // Should optimize at some point to directly use board without
  // get_cell_from_buffer.

  for (int8_t y = 0; y < height; y++) {
    for (int8_t x = 0; x < width; x++) {
      neighbors = 0;
      for (int8_t dx = -1; dx <= 1; dx++) {
        for (int8_t dy = -1; dy <= 1; dy++) {
          if (!(dy == 0 && dx == 0) &&
              get_cell_from_buffer(modulo(x + dx, width),
                                   modulo(y + dy, height))) {
            neighbors++;
          }
        }
      }
      if (neighbors > 3 || neighbors < 2) {
        bitmatrix_unset(board, x, y);
      } else if (neighbors == 3) {
        bitmatrix_set(board, x, y);
      }
    };
  };
}

void run_gol() {
  static uint8_t data[byte_width * height];
  BitMatrix board = {byte_width, data};
  for (uint16_t i = 0; i < height * byte_width; i++) {
    board.data[i] = rand();
  };
  wait_for_wake_release();
  while (1) {
    display_board(board);
    update_board(board);
    if (read_key() == APP_KEY_EXIT) {
      return;
    }
  }
}
