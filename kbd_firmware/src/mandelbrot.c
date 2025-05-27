#include "mandelbrot.h"


#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "display.h"
#include "key_matrix.h"

#define init_scale 32  // pixels per unit

typedef struct {
    float x, y;
} Vec;
Vec add(Vec a, Vec b) { return (Vec){a.x + b.x, a.y + b.y}; }

struct ScreenLoc {
    Vec center;
    float scale;
    uint16_t filled_blocks;
};

static bool point_in_set(Vec point, uint8_t maxiter) {
    float z_re = 0;
    float z_im = 0;
    float z_re_sq;
    float z_im_sq;
    for (uint8_t i = 0; i < maxiter; i++) {
        z_re_sq = z_re * z_re;
        z_im_sq = z_im * z_im;
        if (z_re_sq + z_im_sq > 4) {
            return false;
        }
        z_im = 2 * z_re * z_im + point.y;
        z_re = z_re_sq - z_im_sq + point.x;
    }
    return true;
}

static Vec screen_to_coord(uint8_t x, uint8_t y, struct ScreenLoc *screen) {
    return (Vec){(x - (DISPLAY_WIDTH / 2)) / screen->scale + screen->center.x,
                 (y - (DISPLAY_HEIGHT / 2)) / screen->scale + screen->center.y};
}

static uint8_t get_maxiter(struct ScreenLoc *screen) {
    if (screen->scale <= init_scale * 4) return 20;
    if (screen->scale <= init_scale * 8) return 30;
    return 40;
}


static void display_mandelbrot_block(struct ScreenLoc *screen, uint8_t x,
                                     uint8_t xmax, uint8_t line) {
    for (; x <= xmax; x++) {
        for (uint8_t dy = 0; dy < 8; dy++) {
            if (point_in_set(screen_to_coord(x, line * 8 + dy, screen),
                             get_maxiter(screen))) {
                lcd_drawPixel(x, line * 8 + dy, 1);
            }
        }
        if ((x + 1) % 8 == 0) {
            lcd_display_block(x - 7, line, 8);
            screen->filled_blocks++;
        }
    }
}

static void display_complete_mandelbrot(struct ScreenLoc *screen) {
    lcd_clear_buffer();
    lcd_send_home_command();
    lcd_display();
    screen->filled_blocks = 0;
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8; line++) {
        display_mandelbrot_block(screen, 0, DISPLAY_WIDTH - 1, line);
    }
}

// There probably is some way to integrate these into vertical/horizontal
// functions, but I'm too tired to figure it out...
static void move_up(struct ScreenLoc *screen) {
    screen->center.y -= 16 / screen->scale;
    screen->filled_blocks -= 2 * DISPLAY_WIDTH / 8;
    for (uint8_t line = DISPLAY_HEIGHT / 8 - 1; line > 1; line--) {
        memcpy(displayBuffer[line], displayBuffer[line - 2], DISPLAY_WIDTH);
    }
    memset(displayBuffer, 0, 2 * DISPLAY_WIDTH);
    lcd_display();
    display_mandelbrot_block(screen, 0, DISPLAY_WIDTH - 1, 1);
    display_mandelbrot_block(screen, 0, DISPLAY_WIDTH - 1, 0);
}

static void move_down(struct ScreenLoc *screen) {
    screen->center.y += 16 / screen->scale;
    screen->filled_blocks -= 2 * DISPLAY_WIDTH / 8;
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8 - 2; line++) {
        memcpy(displayBuffer[line], displayBuffer[line + 2], DISPLAY_WIDTH);
    }
    memset(displayBuffer[DISPLAY_HEIGHT / 8 - 2], 0, 2 * DISPLAY_WIDTH);
    lcd_display();
    display_mandelbrot_block(screen, 0, DISPLAY_WIDTH - 1,
                             DISPLAY_HEIGHT / 8 - 2);
    display_mandelbrot_block(screen, 0, DISPLAY_WIDTH - 1,
                             DISPLAY_HEIGHT / 8 - 1);
}

static void move_left(struct ScreenLoc *screen) {
    screen->center.x -= 32 / screen->scale;
    screen->filled_blocks -= 4 * DISPLAY_HEIGHT / 8;
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8; line++) {
        memmove(displayBuffer[line] + 32, displayBuffer[line],
                DISPLAY_WIDTH - 32);
        memset(displayBuffer[line], 0, 32);
    }
    lcd_display();
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8; line++) {
        display_mandelbrot_block(screen, 0, 32, line);
    }
}

static void move_right(struct ScreenLoc *screen) {
    screen->center.x += 32 / screen->scale;
    screen->filled_blocks -= 4 * DISPLAY_HEIGHT / 8;
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8; line++) {
        memmove(displayBuffer[line], displayBuffer[line] + 32,
                DISPLAY_WIDTH - 32);
        memset(displayBuffer[line] + (DISPLAY_WIDTH - 32), 0, 32);
    }
    lcd_display();
    for (uint8_t line = 0; line < DISPLAY_HEIGHT / 8; line++) {
        display_mandelbrot_block(screen, DISPLAY_WIDTH - 33, DISPLAY_WIDTH - 1,
                                 line);
    }
}

void run_mandelbrot() {
    printk("Running Mandelbrot set display\n");
    struct ScreenLoc screen = {
        .center = {0, 0}, .scale = init_scale, .filled_blocks = 0};

    display_complete_mandelbrot(&screen);
    while (current_pressed_keys.wake_pressed){k_msleep(10);}
    k_msleep(50);  // Wait for display to turn on

    while (1) {
        while (1) {
            if (current_pressed_keys.keys[0].row == 1 && current_pressed_keys.keys[0].col == 2) {
                screen.scale *= 2;
                display_complete_mandelbrot(&screen);
                while (current_pressed_keys.n_pressed > 0) {k_msleep(10);}
                k_msleep(100);
                break;
            } else if (current_pressed_keys.keys[0].row == 0 && current_pressed_keys.keys[0].col == 1) {
                screen.scale /= 2;
                display_complete_mandelbrot(&screen);
                while (current_pressed_keys.n_pressed > 0) {
                    k_msleep(10);
                }
                k_msleep(100);
                break;
            } else if (current_pressed_keys.keys[0].row == 1 && current_pressed_keys.keys[0].col == 1) {
                move_left(&screen);
                break;
            } else if (current_pressed_keys.keys[0].row == 1 && current_pressed_keys.keys[0].col == 3) {
                move_right(&screen);
                break;
            } else if (current_pressed_keys.keys[0].row == 0 && current_pressed_keys.keys[0].col == 2) {
                move_up(&screen);
                break;
            } else if (current_pressed_keys.keys[0].row == 2 && current_pressed_keys.keys[0].col == 2) {
                move_down(&screen);
                break;
            } else if (current_pressed_keys.wake_pressed) {
                return;
            }
            k_msleep(10);
        }
        k_msleep(50);
    }
}