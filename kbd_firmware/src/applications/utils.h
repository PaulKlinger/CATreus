#ifndef APP_UTILS_H
#define APP_UTILS_H
#include <stdlib.h>

#include "../key_matrix.h"
#include "strings.h"

void wait_for_wake_release();
void wait_for_wake();

enum AppKey {
  APP_KEY_NONE = 0,
  APP_KEY_UP,
  APP_KEY_DOWN,
  APP_KEY_LEFT,
  APP_KEY_RIGHT,
  APP_KEY_SELECT,
  APP_KEY_BACK,
  APP_KEY_EXIT,
};

enum AppKey read_key();

enum AppKey read_last_key();

typedef struct {
  float x, y;
} Vec;

inline Vec add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y}; }
typedef struct {
  uint8_t x, y;
} u8Vec;

typedef enum Direction { UP, RIGHT, DOWN, LEFT } Direction;
uint8_t randint(uint8_t min, uint8_t max);

typedef struct {
  uint8_t byte_width;
  uint8_t *data;
} BitMatrix;

inline bool bitmatrix_get(BitMatrix matrix, uint8_t x, uint8_t y) {
  return matrix.data[y * matrix.byte_width + x / 8] & (1 << (x % 8));
}

inline void bitmatrix_set(BitMatrix matrix, uint8_t x, uint8_t y) {
  matrix.data[y * matrix.byte_width + x / 8] |= (1 << (x % 8));
}

inline void bitmatrix_unset(BitMatrix matrix, uint8_t x, uint8_t y) {
  matrix.data[y * matrix.byte_width + x / 8] &= ~(1 << (x % 8));
}

void show_game_over_screen(uint16_t points);

inline uint8_t ceil8(float x) {
  uint8_t f = x;  // floor
  return x > f ? f + 1 : f;
}

void rotate_vec(Vec *vec, int8_t angle);

void display_4x4_block(uint8_t x, uint8_t y);

static inline int8_t modulo(int8_t a, int8_t b) {
  // correctly handle negative values
  return (a % b + b) % b;
}

#endif  // APP_UTILS_H
