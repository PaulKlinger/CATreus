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
void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_goto_xpix_y(uint8_t x, uint8_t y);
void lcd_display(void);
void lcd_clear_buffer(void);
void enable_display(void);
void disable_display(void);
bool display_enabled(void);
void lcd_drawPixel(uint8_t x, uint8_t y, uint8_t color);
void lcd_display_block(uint8_t x, uint8_t line, uint8_t width);
void lcd_send_home_command();
void lcd_clrscr(void);


void lcd_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

void lcd_fillRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2, uint8_t color);

void lcd_fillTriangle(int16_t x1, int8_t y1, int16_t x2, int8_t y2,
                      int16_t x3, int8_t y3, uint8_t color);


void lcd_fillCircleSimple(uint8_t center_x, uint8_t center_y, int16_t radius, uint8_t color);

void lcd_drawRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2, uint8_t color);
uint8_t lcd_check_buffer(uint8_t x, uint8_t y);

#endif // DISPLAY_H