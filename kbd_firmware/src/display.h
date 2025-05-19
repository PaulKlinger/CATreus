
#ifdef __cplusplus
extern "C" {
#endif

void display_init(void);
void lcd_puts(const char* s);
void lcd_goto_xpix_y(uint8_t x, uint8_t y);
void lcd_display(void);
void lcd_clear_buffer(void);


#ifdef __cplusplus
}
#endif