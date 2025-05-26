#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

extern uint8_t displayBuffer[DISPLAY_HEIGHT / 8][DISPLAY_WIDTH];
#define WHITE            0x01
#define BLACK            0x00


void display_init(void);
void lcd_puts(const char* s);
void lcd_goto_xpix_y(uint8_t x, uint8_t y);
void lcd_display(void);
void lcd_clear_buffer(void);
void enable_display(void);
void disable_display(void);
bool display_enabled(void);
void lcd_drawPixel(uint8_t x, uint8_t y, uint8_t color);
void lcd_display_block(uint8_t x, uint8_t line, uint8_t width);
void lcd_send_home_command();

#endif // DISPLAY_H