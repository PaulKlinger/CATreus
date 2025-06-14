#include "display.h"

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

const char FONT[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // sp
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00},  // !
    {0x00, 0x00, 0x07, 0x00, 0x07, 0x00},  // "
    {0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14},  // #
    {0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12},  // $
    {0x00, 0x62, 0x64, 0x08, 0x13, 0x23},  // %
    {0x00, 0x36, 0x49, 0x55, 0x22, 0x50},  // &
    {0x00, 0x00, 0x05, 0x03, 0x00, 0x00},  // '
    {0x00, 0x00, 0x1c, 0x22, 0x41, 0x00},  // (
    {0x00, 0x00, 0x41, 0x22, 0x1c, 0x00},  // )
    {0x00, 0x14, 0x08, 0x3E, 0x08, 0x14},  // *
    {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08},  // +
    {0x00, 0x00, 0x00, 0xA0, 0x60, 0x00},  // ,
    {0x00, 0x08, 0x08, 0x08, 0x08, 0x08},  // -
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00},  // .
    {0x00, 0x20, 0x10, 0x08, 0x04, 0x02},  // /
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E},  // 0
    {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00},  // 1
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46},  // 2
    {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31},  // 3
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10},  // 4
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39},  // 5
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30},  // 6
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03},  // 7
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36},  // 8
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E},  // 9
    {0x00, 0x00, 0x36, 0x36, 0x00, 0x00},  // :
    {0x00, 0x00, 0x56, 0x36, 0x00, 0x00},  // ;
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x00},  // <
    {0x00, 0x14, 0x14, 0x14, 0x14, 0x14},  // =
    {0x00, 0x00, 0x41, 0x22, 0x14, 0x08},  // >
    {0x00, 0x02, 0x01, 0x51, 0x09, 0x06},  // ?
    {0x00, 0x32, 0x49, 0x59, 0x51, 0x3E},  // @
    {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C},  // A
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36},  // B
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22},  // C
    {0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C},  // D
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41},  // E
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01},  // F
    {0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A},  // G
    {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F},  // H
    {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00},  // I
    {0x00, 0x20, 0x40, 0x41, 0x3F, 0x01},  // J
    {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41},  // K
    {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40},  // L
    {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F},  // M
    {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F},  // N
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E},  // O
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06},  // P
    {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E},  // Q
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46},  // R
    {0x00, 0x46, 0x49, 0x49, 0x49, 0x31},  // S
    {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01},  // T
    {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F},  // U
    {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F},  // V
    {0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F},  // W
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63},  // X
    {0x00, 0x07, 0x08, 0x70, 0x08, 0x07},  // Y
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43},  // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41, 0x00},  // [
    {0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55},  // backslash
    {0x00, 0x00, 0x41, 0x41, 0x7F, 0x00},  // ]
    {0x00, 0x04, 0x02, 0x01, 0x02, 0x04},  // ^
    {0x00, 0x40, 0x40, 0x40, 0x40, 0x40},  // _
    {0x00, 0x00, 0x01, 0x02, 0x04, 0x00},  // '
    {0x00, 0x20, 0x54, 0x54, 0x54, 0x78},  // a
    {0x00, 0x7F, 0x48, 0x44, 0x44, 0x38},  // b
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x20},  // c
    {0x00, 0x38, 0x44, 0x44, 0x48, 0x7F},  // d
    {0x00, 0x38, 0x54, 0x54, 0x54, 0x18},  // e
    {0x00, 0x08, 0x7E, 0x09, 0x01, 0x02},  // f
    {0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C},  // g
    {0x00, 0x7F, 0x08, 0x04, 0x04, 0x78},  // h
    {0x00, 0x00, 0x44, 0x7D, 0x40, 0x00},  // i
    {0x00, 0x40, 0x80, 0x84, 0x7D, 0x00},  // j
    {0x00, 0x7F, 0x10, 0x28, 0x44, 0x00},  // k
    {0x00, 0x00, 0x41, 0x7F, 0x40, 0x00},  // l
    {0x00, 0x7C, 0x04, 0x18, 0x04, 0x78},  // m
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x78},  // n
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38},  // o
    {0x00, 0xFC, 0x24, 0x24, 0x24, 0x18},  // p
    {0x00, 0x18, 0x24, 0x24, 0x18, 0xFC},  // q
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x08},  // r
    {0x00, 0x48, 0x54, 0x54, 0x54, 0x20},  // s
    {0x00, 0x04, 0x3F, 0x44, 0x40, 0x20},  // t
    {0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C},  // u
    {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C},  // v
    {0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C},  // w
    {0x00, 0x44, 0x28, 0x10, 0x28, 0x44},  // x
    {0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C},  // y
    {0x00, 0x44, 0x64, 0x54, 0x4C, 0x44},  // z
    {0x00, 0x00, 0x08, 0x77, 0x41, 0x00},  // {
    {0x00, 0x00, 0x00, 0x63, 0x00, 0x00},  // ¦
    {0x00, 0x00, 0x41, 0x77, 0x08, 0x00},  // }
    {0x00, 0x08, 0x04, 0x08, 0x08, 0x04},  // ~
};

#define LCD_DISP_OFF 0xAE
#define LCD_DISP_ON 0xAF

const uint8_t init_sequence[] = {
    // Initialization Sequence
    LCD_DISP_OFF,  // Display OFF (sleep mode)
    0x20,
    0b00,  // Set Memory Addressing Mode
    // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
    // 10=Page Addressing Mode (RESET); 11=Invalid
    0xB0,  // Set Page Start Address for Page Addressing Mode, 0-7
    0xC0,  // Set COM Output Scan Direction
    0x00,  // --set low column address
    0x10,  // --set high column address
    0x40,  // --set start line address
    0x81,
    0xFF,  // Set contrast control register
    0xA0,  // Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
    0xA6,  // Set display mode. A6=Normal; A7=Inverse
    0xA8,
    0x3F,  // Set multiplex ratio(1 to 64)
    0xA4,  // Output RAM to Display
    // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
    0xD3,
    0x00,        // Set display offset. 00 = no offset
    0xD5,        // --set display clock divide ratio/oscillator frequency
    0b11110000,  // --set divide ratio
    0xD9,
    0x22,  // Set pre-charge period
    0xDA,
    0x12,  // Set com pins hardware configuration
    0xDB,  // --set vcomh
    0x20,  // 0x20,0.77xVcc
    0x8D,
    0x14,  // Set DC-DC enable
    LCD_DISP_ON,
};

#define I2C21_NODE DT_NODELABEL(display)
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C21_NODE);
static const struct device *disp_ldsw =
    DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo1));

static struct {
  uint8_t x;
  uint8_t y;
} cursorPosition;

uint8_t displayBuffer[DISPLAY_HEIGHT / 8][DISPLAY_WIDTH];

void init_i2c(void) {
  while (!device_is_ready(dev_i2c.bus)) {
    printk("I2C bus %s is not ready\n", dev_i2c.bus->name);
    k_sleep(K_MSEC(100));
  };
}

int lcd_command(const uint8_t *cmds, size_t len) {
  struct i2c_msg mode_msg;
  uint8_t is_command = 0x00;
  mode_msg.buf = &is_command;
  mode_msg.len = 1;
  mode_msg.flags = I2C_MSG_WRITE;

  struct i2c_msg data_msg;
  data_msg.buf = (uint8_t *)cmds;
  data_msg.len = len;
  data_msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

  struct i2c_msg msgs[] = {mode_msg, data_msg};
  int ret = i2c_transfer(dev_i2c.bus, msgs, 2, dev_i2c.addr);
  if (ret != 0) {
    printk("Error %d: failed to write command to the display\n", ret);
  }
  return ret;
}

int lcd_data(const uint8_t *data, size_t len) {
  struct i2c_msg mode_msg;
  uint8_t is_data = 0x40;
  mode_msg.buf = &is_data;
  mode_msg.len = 1;
  mode_msg.flags = I2C_MSG_WRITE;

  struct i2c_msg data_msg;
  data_msg.buf = (uint8_t *)data;
  data_msg.len = len;
  data_msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

  struct i2c_msg msgs[] = {mode_msg, data_msg};
  int ret = i2c_transfer(dev_i2c.bus, msgs, 2, dev_i2c.addr);
  if (ret != 0) {
    printk("Error %d: failed to write data to the display\n", ret);
  }
  return ret;
}

void lcd_goto_xpix_y(uint8_t x, uint8_t y) {
  cursorPosition.x = x;
  cursorPosition.y = y;
}

void lcd_gotoxy(uint8_t x, uint8_t y) {
  x = x * sizeof(FONT[0]);
  lcd_goto_xpix_y(x, y);
}

void lcd_send_goto_xpix_y(uint8_t x, uint8_t y) {
  cursorPosition.x = x;
  cursorPosition.y = y;
  uint8_t commandSequence[] = {0x22, y, 0x07, 0x21, x, 0x7f};
  lcd_command(commandSequence, sizeof(commandSequence));
}

void lcd_send_home_command() { lcd_send_goto_xpix_y(0, 0); }

void lcd_putc(char c) {
  // mapping char
  c -= ' ';
  for (uint8_t i = 0; i < sizeof(FONT[0]); i++) {
    // load bit-pattern from flash
    displayBuffer[cursorPosition.y][cursorPosition.x + i] = FONT[(uint8_t)c][i];
  }
  cursorPosition.x += sizeof(FONT[0]);
}

void lcd_puts(const char *s) {
  while (*s) {
    if (*s == '\n') {
      cursorPosition.x = 0;
      cursorPosition.y++;
      lcd_goto_xpix_y(cursorPosition.x, cursorPosition.y);
    } else {
      lcd_putc(*s);
    }
    s++;
  }
}

void lcd_putc_invert(char c) {
  // mapping char
  c -= ' ';
  for (uint8_t i = 0; i < sizeof(FONT[0]); i++) {
    // load bit-pattern from flash
    displayBuffer[cursorPosition.y][cursorPosition.x + i] =
        ~FONT[(uint8_t)c][i];
  }
  cursorPosition.x += sizeof(FONT[0]);
}

void lcd_puts_invert(const char *s) {
  while (*s) {
    if (*s == '\n') {
      cursorPosition.x = 0;
      cursorPosition.y++;
      lcd_goto_xpix_y(cursorPosition.x, cursorPosition.y);
    } else {
      lcd_putc_invert(*s);
    }
    s++;
  }
}

void lcd_clear_buffer() {
  for (uint8_t i = 0; i < DISPLAY_HEIGHT / 8; i++) {
    memset(displayBuffer[i], 0x00, sizeof(displayBuffer[i]));
  }
}

uint8_t lcd_check_buffer(uint8_t x, uint8_t y) {
  return displayBuffer[(y / (DISPLAY_HEIGHT / 8))][x] &
         (1 << (y % (DISPLAY_HEIGHT / 8)));
}

void lcd_display() {
  lcd_send_home_command();
  lcd_data(&displayBuffer[0][0], DISPLAY_WIDTH * DISPLAY_HEIGHT / 8);
}
void lcd_clrscr(void) {
  lcd_clear_buffer();
  lcd_display();
  lcd_send_home_command();
}

bool display_enabled(void) { return regulator_is_enabled(disp_ldsw); }

void enable_display(void) {
  // Don't enable display if it is already enabled
  // (register keeps the value and would enable it again the next time it's
  // disabled)
  if (!display_enabled()) {
    int ret = regulator_enable(disp_ldsw);
    if (ret != 0) {
      printk("Error %d: failed to enable display LDO\n", ret);
    }
  }
}

void disable_display(void) {
  if (display_enabled()) {
    int ret = regulator_disable(disp_ldsw);
    if (ret != 0) {
      printk("Error %d: failed to disable display LDO\n", ret);
    }
  }
}

void display_init(void) {
  lcd_clear_buffer();
  enable_display();
  k_msleep(50);
  init_i2c();

  int ret = lcd_command(init_sequence, sizeof(init_sequence));
  if (ret != 0) {
    printk("Error %d: failed to write to the display\n", ret);
  }
  k_msleep(50);  // Wait for display to turn on
  lcd_display();
}

void lcd_drawPixel(uint8_t x, uint8_t y, uint8_t color) {
  if (color == WHITE) {
    displayBuffer[(y / (DISPLAY_HEIGHT / 8))][x] |=
        (1 << (y % (DISPLAY_HEIGHT / 8)));
  } else {
    displayBuffer[(y / (DISPLAY_HEIGHT / 8))][x] &=
        ~(1 << (y % (DISPLAY_HEIGHT / 8)));
  }
}

void lcd_display_block(uint8_t x, uint8_t line, uint8_t width) {
  if (line > (DISPLAY_HEIGHT / 8 - 1) || x > DISPLAY_WIDTH - 1) {
    return;
  }
  if (x + width > DISPLAY_WIDTH) {  // no -1 here, x alone is width 1
    width = DISPLAY_WIDTH - x;
  }
  lcd_send_goto_xpix_y(x, line);
  lcd_data(&displayBuffer[line][x], width);
}

void lcd_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                  uint8_t color) {
  if (x1 > DISPLAY_WIDTH - 1 || x2 > DISPLAY_WIDTH - 1 ||
      y1 > DISPLAY_HEIGHT - 1 || y2 > DISPLAY_HEIGHT - 1)
    return;
  int dx = abs((int)x2 - x1), sx = x1 < x2 ? 1 : -1;
  int dy = -abs((int)y2 - y1), sy = y1 < y2 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  while (1) {
    lcd_drawPixel(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    e2 = 2 * err;
    if (e2 > dy) {
      err += dy;
      x1 += sx;
    } /* e_xy+e_x > 0 */
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    } /* e_xy+e_y < 0 */
  }
}

void lcd_fillRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2,
                  uint8_t color) {
  for (uint8_t i = 0; i <= (py2 - py1); i++) {
    lcd_drawLine(px1, py1 + i, px2, py1 + i, color);
  }
}

void lcd_fillTriangle(int16_t x1, int8_t y1, int16_t x2, int8_t y2, int16_t x3,
                      int8_t y3, uint8_t color) {
  // Negative and too large coords are allowed, only the visible part will
  // be drawn

  // calc bounds for increased performance (todo: is there a better way?)
  int16_t xmin = (x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3));
  if (xmin < 0) xmin = 0;
  int16_t xmax = (x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3));
  if (xmax > DISPLAY_WIDTH - 1) xmax = DISPLAY_WIDTH - 1;

  int8_t ymin = (y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3));
  if (ymin < 0) ymin = 0;
  int8_t ymax = (y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3));
  if (ymax > DISPLAY_HEIGHT - 1) ymax = DISPLAY_HEIGHT - 1;

  for (uint8_t x = xmin; x <= xmax; x++) {
    for (uint8_t y = ymin; y <= ymax; y++) {
      // point in triangle code from John Bananas on stackoverflow
      // https://stackoverflow.com/a/9755252/7089433
      int8_t p1x = x - x1;
      int8_t p1y = y - y1;
      uint8_t s12 = (x2 - x1) * p1y - (y2 - y1) * p1x > 0;
      if (((x3 - x1) * p1y - (y3 - y1) * p1x > 0) == s12) continue;
      if (((x3 - x2) * (y - y2) - (y3 - y2) * (x - x2) > 0) != s12) continue;
      lcd_drawPixel(x, y, color);
    }
  }
}

void lcd_fillCircleSimple(uint8_t center_x, uint8_t center_y, int16_t radius,
                          uint8_t color) {
  for (int16_t dx = -radius; dx <= radius; dx++) {
    for (int16_t dy = -radius; dy <= radius; dy++) {
      if (dx * dx + dy * dy < radius * radius) {
        if (center_x + dx >= DISPLAY_WIDTH || center_x + dx < 0 ||
            center_y + dy >= DISPLAY_HEIGHT || center_y + dy < 0)
          continue;
        lcd_drawPixel(center_x + dx, center_y + dy, color);
      }
    }
  }
}

void lcd_drawRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2,
                  uint8_t color) {
  lcd_drawLine(px1, py1, px2, py1, color);
  lcd_drawLine(px2, py1, px2, py2, color);
  lcd_drawLine(px2, py2, px1, py2, color);
  lcd_drawLine(px1, py2, px1, py1, color);
}
