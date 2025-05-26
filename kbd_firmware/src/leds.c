#include "leds.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

K_THREAD_STACK_DEFINE(led_thread_stack, 1024);
struct k_thread led_thread_data;

static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(led2), gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(led3), gpios),
};

void init_leds(void) {
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        while (!device_is_ready(leds[i].port)) {}
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
    }
}


void led_on(uint8_t led) {
    gpio_pin_set_dt(&leds[led], 1);
}

void led_off(uint8_t led) {
    gpio_pin_set_dt(&leds[led], 0);
}

void advertising_anim() {
    int8_t dir = 1;
    int8_t led = 0;
    while (1) {
        led_on(led);
        k_msleep(150);
        led_off(led);
        led += dir;
        if (led >= ARRAY_SIZE(leds) || led < 0) {
            dir *= -1;
            led += dir;
        }
    }
}

void led_start_advertising_anim(void) {
    k_thread_create(&led_thread_data, led_thread_stack,
                    K_THREAD_STACK_SIZEOF(led_thread_stack), advertising_anim, NULL,
                    NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
}

void led_stop_anim(void) {
    k_thread_abort(&led_thread_data);
    for (int i = 0; i < ARRAY_SIZE(leds); i++) {
        led_off(i);
    }
}