#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

void init_leds(void);
void led_on(uint8_t led);
void led_off(uint8_t led);

void led_start_advertising_anim(void);
void led_stop_anim(void);

#endif // LEDS_H