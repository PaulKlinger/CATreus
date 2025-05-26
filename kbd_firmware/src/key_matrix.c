
#include "key_matrix.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)


const struct gpio_dt_spec gpio_rows[] = {
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, r0_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, r1_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, r2_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, r3_gpios),
};

const struct gpio_dt_spec gpio_cols[] = {
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c0_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c1_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c2_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c3_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c4_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c5_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c6_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c7_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c8_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c9_gpios),
    GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, c10_gpios),
};

const struct gpio_dt_spec wake_btn = GPIO_DT_SPEC_GET(DT_NODELABEL(wakebtn), gpios);

static struct gpio_callback button_cb_data;
gpio_port_pins_t detect_pins = BIT(0) |
                  BIT(1) |
                  BIT(2) |
                  BIT(3) |
                  BIT(4);

gpio_port_pins_t drive_pins = BIT(0) |
                  BIT(1) |
                  BIT(2) |
                  BIT(3) |
                  BIT(4) |
                  BIT(5) |
                  BIT(6) |
                  BIT(7) |
                  BIT(8) |
                  BIT(9) |
                  BIT(10);

static K_SEM_DEFINE(wake_sem, 0, 1);



static inline void disable_row_interrupts(){
    for (int i = 0; i < 5; i++) {
        gpio_pin_interrupt_configure(wake_btn.port, i, GPIO_INT_DISABLE);
    }
}

static inline void enable_row_interrupts(){
    for (int i = 0; i < 5; i++) {
        gpio_pin_interrupt_configure(wake_btn.port, i, GPIO_INT_LEVEL_ACTIVE);
    }
}
// Interrupt handler function
static void gpio_isr_callback(const struct device *dev,
                                  struct gpio_callback *callback,
                                  uint32_t pins) {
    disable_row_interrupts();
    k_sem_give(&wake_sem); // Signal the thread to wake up
}


void init_key_matrix(void) {
    // Initialize the key matrix
    
    
    gpio_pin_configure_dt(&wake_btn, GPIO_INPUT | GPIO_ACTIVE_LOW | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&wake_btn, GPIO_INT_LEVEL_ACTIVE);
    
    for (int i = 0; i < ARRAY_SIZE(gpio_rows); i++) {
        gpio_pin_configure_dt(&gpio_rows[i], GPIO_INPUT | GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN);
        gpio_pin_interrupt_configure_dt(&gpio_rows[i], GPIO_INT_LEVEL_ACTIVE);
    }
    
    gpio_init_callback(&button_cb_data, gpio_isr_callback, detect_pins);
    int ret = gpio_add_callback(wake_btn.port, &button_cb_data);
    printk("gpio_add_callback ret %d\n", ret);
    
    for (int i = 0; i < ARRAY_SIZE(gpio_cols); i++) {
        gpio_pin_configure_dt(&gpio_cols[i], GPIO_OUTPUT | GPIO_ACTIVE_HIGH);
    }

}

struct pressed_keys read_key_matrix(void) {
    struct pressed_keys res = {0};
    res.wake_pressed = wake_pressed();
    uint8_t row, col;
    
    for (col = 0; col < ARRAY_SIZE(gpio_cols); col++) {
        gpio_pin_set_dt(&gpio_cols[col], 1);
        for (row = 0; row < ARRAY_SIZE(gpio_rows); row++) {
            if (gpio_pin_get_dt(&gpio_rows[row]) == 1) {
                res.keys[res.n_pressed].row = row;
                res.keys[res.n_pressed].col = col;
                res.n_pressed++;
                if (res.n_pressed >= MAX_PRESSED_KEYS) {
                    break;
                }
            }
        }
        gpio_pin_set_dt(&gpio_cols[col], 0);
    }
    
    return res;
}

bool wake_pressed(void) {
    return gpio_pin_get_dt(&wake_btn);
}

int wait_for_key(int timeout_ms) {
    enable_row_interrupts();
    k_sem_take(&wake_sem, K_NO_WAIT);
    gpio_port_set_bits(gpio_cols[0].port, drive_pins);
    int ret = k_sem_take(&wake_sem, K_MSEC(timeout_ms));
    gpio_port_clear_bits(gpio_cols[0].port, drive_pins);
    return ret;
}

bool eq_pressed_keys(struct pressed_keys a, struct pressed_keys b) {
    if (a.n_pressed != b.n_pressed || a.wake_pressed != b.wake_pressed) {
        return false;
    }
    
    for (int i = 0; i < a.n_pressed; i++) {
        if (a.keys[i].row != b.keys[i].row || a.keys[i].col != b.keys[i].col) {
            return false;
        }
    }
    
    return true;
}