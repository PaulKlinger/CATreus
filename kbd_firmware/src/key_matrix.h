#ifndef KEY_MATRIX_H
#define KEY_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

struct pressed_keys read_key_matrix(void);

void init_key_matrix(void);

bool wake_pressed(void);

int wait_for_key(int timeout_ms);

#define MAX_PRESSED_KEYS 6

#endif // KEY_MATRIX_H