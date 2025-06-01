#include "utils.h"
#include <zephyr/kernel.h>
#include "../display.h"

void wait_for_wake_release() {
    while (wake_pressed()) {
        k_msleep(10);
    }
    k_msleep(20);
}
void wait_for_wake(){
    while (!wake_pressed()) {
        k_msleep(10);
    }
}

enum AppKey translate_pressed_keys(struct pressed_keys pressed) {
    if (pressed.n_pressed == 0 &&
        !pressed.wake_pressed) {
        return APP_KEY_NONE;
    } else if (pressed.wake_pressed) {
        return APP_KEY_EXIT;
    } else if (pressed.keys[0].row == 1 &&
               pressed.keys[0].col == 2) {
        return APP_KEY_SELECT;
    } else if (pressed.keys[0].row == 0 &&
               pressed.keys[0].col == 1) {
        return APP_KEY_BACK;
    } else if (pressed.keys[0].row == 1 &&
               pressed.keys[0].col == 1) {
        return APP_KEY_LEFT;
    } else if (pressed.keys[0].row == 1 &&
               pressed.keys[0].col == 3) {
        return APP_KEY_RIGHT;
    } else if (pressed.keys[0].row == 0 &&
               pressed.keys[0].col == 2) {
        return APP_KEY_UP;
    } else if (pressed.keys[0].row == 2 &&
               pressed.keys[0].col == 2) {
        return APP_KEY_DOWN;
    } else {
        return APP_KEY_NONE;
    }
}

enum AppKey read_key() {
    return translate_pressed_keys(current_pressed_keys);
}

enum AppKey read_last_key() {
    if (current_pressed_keys.n_pressed > 0 || current_pressed_keys.wake_pressed) {
        return translate_pressed_keys(last_pressed_keys);
    } else {
        return translate_pressed_keys(last_pressed_keys);
    }
}

uint8_t randint(uint8_t min, uint8_t max) {
    uint8_t ret = rand();
    while (ret < min || ret > max) ret = rand();
    return ret;
}



void show_game_over_screen(uint16_t points) {
    lcd_clear_buffer();
    lcd_gotoxy(6,2);
    lcd_puts(string_game_over);
    char points_str[6];
    sprintf(points_str, "%d", points);
    lcd_gotoxy(6,3);
    lcd_puts(points_str);
    lcd_puts(string_points);
    lcd_gotoxy(3,5);
    lcd_puts(string_press_to_return);
    lcd_display();
    wait_for_wake();
}

void rotate_vec(Vec *vec, int8_t angle) {
    // Rotates vec in steps of 1 degree.
    // Rotating by 2 degree at a time would increase precision if I never
    // need to rotate in finer steps
    float x_temp;
    bool dir = angle > 0;
    angle = abs(angle);
    for (; angle > 0; angle--) {
        x_temp = vec->x;
        vec->x = 0.999848f * vec->x - (dir ? 1 : -1) * 0.0174524f * vec->y;
        vec->y = (dir ? 1 : -1) * 0.0174524f * x_temp + 0.999848f * vec->y;
    }
}


void display_4x4_block(uint8_t x, uint8_t y) {
    lcd_display_block(x*4, y/2, 4);
}
