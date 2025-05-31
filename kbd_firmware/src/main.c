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
#include "mandelbrot.h"
#include "ui.h"
#include "nvs.h"

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
    k_msleep(50);
    init_leds();
    
    init_ui();
    ui_send_startup();

    nvs_init();
    uint16_t n_boot = nvs_read_n_boot();
    nvs_store_n_boot(n_boot + 1);

    int err;
    err = init_bluetooth();
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Init key matrix\n");
    init_key_matrix();
    uint32_t last_active_time = k_uptime_seconds();
    uint32_t last_no_pressed_time = last_active_time;

    while (1) {
        uint32_t seconds_since_active = k_uptime_seconds() - last_active_time;
        uint32_t seconds_since_no_pressed = k_uptime_seconds() - last_no_pressed_time;
        if ((seconds_since_active > DEEP_SLEEP_TIMEOUT_S) ||
            ((seconds_since_active > DEEP_SLEEP_ADVERTISING_TIMEOUT_S) &&
             ble_is_advertising()) ||
            (seconds_since_no_pressed > DEEP_SLEEP_NO_PRESSED_TIMEOUT_S)) {
            printk("No activity for %ds, going to deep sleep\n",
                   seconds_since_active);
            ui_send_wake_and_key((struct key_coord){1, 6});  // S
        }

        read_key_matrix();

        if (!eq_pressed_keys(last_pressed_keys, current_pressed_keys)) {
            last_active_time = k_uptime_seconds();

            if (current_pressed_keys.wake_pressed) {
                if (current_pressed_keys.n_pressed > 0) {
                    ui_send_wake_and_key(current_pressed_keys.keys[0]);
                } else if (!last_pressed_keys.wake_pressed) {
                    ui_send_wake();
                }
            } else {
                // applications take exclusive control of the keys
                // so only send them to the host if they are not running
                if (application_running) {
                    // send empty to clear any previous keys
                    send_encoded_keys((struct encoded_keys){0});  // send empty keys
                } else {
                    struct encoded_keys encoded_keys = get_encoded_keys();
                    send_encoded_keys(encoded_keys);
                }
            }
            if (ui_active() && current_pressed_keys.n_pressed > 0) {
                // TODO: doesn't really make sense, maybe just get rid of the key message?
                ui_send_key(current_pressed_keys.keys[0]);
            }
        }

        if (current_pressed_keys.n_pressed == 0 &&
            !current_pressed_keys.wake_pressed) {
            last_no_pressed_time = k_uptime_seconds();
        }

        if (current_pressed_keys.n_pressed > 0 ||
            current_pressed_keys.wake_pressed) {
            // if keys are pressed always sleep for 50ms
            // (we can't use the level interrupt here)
            // TODO: could switch to edge interrupt in this case??
            k_msleep(50);
        } else {
            ret = wait_for_key(2000);
        }
    }

    return 0;
}
