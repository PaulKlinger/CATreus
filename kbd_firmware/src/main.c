/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include "bluetooth.h"
#include "config.h"
#include "key_layout.h"
#include "key_matrix.h"
#include "leds.h"
#include "ui.h"
#include "mandelbrot.h"

void i2c_scanner(const struct device *bus) {
    uint8_t error = 0u;
    uint8_t dst;
    uint8_t i2c_dev_cnt = 0;
    struct i2c_msg msgs[1];
    msgs[0].buf = &dst;
    msgs[0].len = 1U;
    msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

    /* Use the full range of I2C address for display purpose */
    for (uint16_t x = 0; x <= 0x7f; x++) {
        /* New line every 0x10 address */
        if (x % 0x10 == 0) {
            printk("|\n0x%02x| ", x);
        }
        /* Range the test with the start and stop value configured in the
         * kconfig */
        if (x >= 0 && x <= 0x7f) {
            /* Send the address to read from */
            error = i2c_transfer(bus, &msgs[0], 1, x);
            /* I2C device found on current address */
            if (error == 0) {
                printk("0x%02x ", x);
                i2c_dev_cnt++;
            } else {
                printk(" --  ");
            }
        } else {
            /* Scan value out of range, not scanned */
            printk("     ");
        }
    }
    printk("|\n");
    printk("\nI2C device(s) found on the bus: %d\nScanning done.\n\n",
           i2c_dev_cnt);
    printk(
        "Find the registered I2C address on: "
        "https://i2cdevices.org/addresses\n\n");
}

int main(void) {
    int ret;
    printk("Starting wrls atreus\n");
    init_leds();

    for (int i = 0; i < 2; i++) {
        led_on(0);
        k_msleep(100);
        led_off(0);
        k_msleep(100);
    }

    int err;

    err = init_bluetooth();
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Init key matrix\n");
    init_key_matrix();
    init_ui();
    struct pressed_keys last_pressed_keys = {0};
    uint32_t last_active_time = k_uptime_seconds();

    while (1) {
        uint32_t seconds_since_active = k_uptime_seconds() - last_active_time;
		printk("Seconds since active: %d\n", seconds_since_active);
        if ((seconds_since_active > DEEP_SLEEP_TIMEOUT_S) ||
            ((seconds_since_active > DEEP_SLEEP_ADVERTISING_TIMEOUT_S) &&
             ble_is_advertising())) {
            printk("No keys pressed for %ds, going to deep sleep\n",
                   DEEP_SLEEP_TIMEOUT_S);
            ui_send_wake_and_key((struct key_coord){1, 6});  // S
        }

        struct pressed_keys keys = read_key_matrix();
        if (keys.n_pressed > 0) {
            printk("pressed keys: ");
            for (int i = 0; i < keys.n_pressed; i++) {
                printk("%d,%d ", keys.keys[i].row, keys.keys[i].col);
            }
            printk("\n");
        }

        printk("wake pressed: %d, n_pressed: %d\n", keys.wake_pressed,
               keys.n_pressed);
        if (!eq_pressed_keys(last_pressed_keys, keys)) {
            last_active_time = k_uptime_seconds();
            printk("keys changed, new n: %d\n", keys.n_pressed);
            struct encoded_keys encoded_keys = encode_keys(keys);
            send_encoded_keys(encoded_keys);

            if (keys.wake_pressed) {
                if (keys.n_pressed > 0) {
					if (keys.keys[0].row == 2 && keys.keys[0].col == 7) {
                        // TODO: move this to UI, add applications page
                        send_encoded_keys((struct encoded_keys) {0});
                        suspend_ui();
						run_mandelbrot();
                        resume_ui();
					}
                    ui_send_wake_and_key(keys.keys[0]);
                } else if (!last_pressed_keys.wake_pressed) {
                    ui_send_wake();
                }
            } else if (ui_active() && keys.n_pressed > 0) {
                ui_send_key(keys.keys[0]);
            }
        }
        last_pressed_keys = keys;

        if (keys.n_pressed > 0 || keys.wake_pressed) {
            // if keys are pressed always sleep for 50ms
            // (we can't use the level interrupt here)
            // TODO: could switch to edge interrupt in this case??
            //       or go to deep sleep if keys don't change for a long time
            k_msleep(50);
        } else {
            ret = wait_for_key(2000);
            printk("wake sem ret %d\n", ret);
        }
    }

    return 0;
}
