/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>

 #include <zephyr/types.h>
 #include <stddef.h>
 #include <string.h>
 #include <errno.h>
 #include <zephyr/sys/printk.h>
 #include <zephyr/sys/byteorder.h>
 #include <zephyr/kernel.h>
 
 #include <zephyr/settings/settings.h>
 
 #include <zephyr/bluetooth/bluetooth.h>
 #include <zephyr/bluetooth/hci.h>
 #include <zephyr/bluetooth/conn.h>
 #include <zephyr/bluetooth/uuid.h>
 #include <zephyr/bluetooth/gatt.h>

#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>


#include <zephyr/drivers/i2c.h>

#include "config.h"
#include "key_layout.h"
#include "hid.h"
#include "ui.h"

#include "display.h"
#include "key_matrix.h"
#include "bluetooth.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   200

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_NODELABEL(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);



static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));


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
			printk("|\n0x%02x| ",x);	
		}
		/* Range the test with the start and stop value configured in the kconfig */
		if (x >= 0 && x <= 0x7f)	{	
			/* Send the address to read from */
			error = i2c_transfer(bus, &msgs[0], 1, x);
				/* I2C device found on current address */
				if (error == 0) {
					printk("0x%02x ",x);
					i2c_dev_cnt++;
				}
				else {
					printk(" --  ");
				}
		} else {
			/* Scan value out of range, not scanned */
			printk("     ");
		}
	}
	printk("|\n");
	printk("\nI2C device(s) found on the bus: %d\nScanning done.\n\n", i2c_dev_cnt);
	printk("Find the registered I2C address on: https://i2cdevices.org/addresses\n\n");
}


int main(void)
{
	int ret;
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}
	
	printk("Starting wrls atreus\n");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	for (int i = 0; i < 5; i++) {
		gpio_pin_toggle_dt(&led);
		k_msleep(200);
	}
	disable_display();
	
	int err;

	err = init_bluetooth();
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Init key matrix\n");
	init_key_matrix();
	while (1) {
		struct pressed_keys keys = read_key_matrix();
		if (keys.n_pressed > 0) {
			printk("pressed keys: ");
			for (int i = 0; i < keys.n_pressed; i++) {
				printk("%d,%d ", keys.keys[i].row, keys.keys[i].col);
			}
			printk("\n");
		}

		if (is_waiting_for_passkey_confirmation()) {
			if (keys.n_pressed == 1 && keys.keys[0].row == 2 && keys.keys[0].col == 2) {
				// Confirm passkey
				confirm_passkey();
			} else if (keys.n_pressed == 1 && keys.keys[0].row == 1 && keys.keys[0].col == 7) {
				// Reject passkey
				reject_passkey();
			}
		}
		struct encoded_keys encoded_keys = encode_keys(keys);
		send_encoded_keys(encoded_keys);


		if (wake_pressed()) {
			if (keys.keys[1].row == 1 && keys.keys[1].col == 6) {
				while (wake_pressed()) {
					k_msleep(10);
				}
				k_msleep(100);
				printk("Going to sleep (ship mode)\n");
				ret = regulator_parent_ship_mode(regulators);
				printk("ship mode ret %d\n", ret);
				printk("Should be asleep!\n");
				k_msleep(100);
				printk("Should have been asleep 100ms!\n");
				k_sleep(K_FOREVER);
			}
			else if (display_enabled()) {
				printk("disable display\n");
				disable_display();
			} else {
				printk("enable display\n");
				display_init();
				show_debug_page();
				printk("enable display ldo ret %d\n", ret);
				// TODO: UI thread
			}
		}
		if (keys.n_pressed > 0) {
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
