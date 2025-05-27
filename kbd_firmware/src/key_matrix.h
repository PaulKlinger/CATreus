#ifndef KEY_MATRIX_H
#define KEY_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

void read_key_matrix(void);

void init_key_matrix(void);

bool wake_pressed(void);

int wait_for_key(int timeout_ms);

extern struct pressed_keys current_pressed_keys;
extern struct pressed_keys last_pressed_keys;

#endif // KEY_MATRIX_H