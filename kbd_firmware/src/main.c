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



#include "display.h"
#include "key_matrix.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   200

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_NODELABEL(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


static const struct device *disp_ldsw = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo1));


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s, err 0x%02x %s\n", addr,
		       err, bt_hci_err_to_str(err));
		return;
	}

	printk("Connected %s\n", addr);

	if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
		printk("Failed to set security\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s, reason 0x%02x %s\n", addr,
	       reason, bt_hci_err_to_str(reason));
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %s(%d)\n", addr, level,
		       bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	hid_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

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

	ret = regulator_is_enabled(disp_ldsw);
	if (ret) {
		printk("display ldsw enabled, disabling\n");
		regulator_disable(disp_ldsw);
	} else {
		printk("display ldsw not enabled\n");
	}
	
	int err;

	err = bt_enable(bt_ready);
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
		struct encoded_keys encoded_keys = encode_keys(keys);
		send_encoded_keys(encoded_keys);


		if (wake_pressed()) {
			if (regulator_is_enabled(disp_ldsw)) {
				printk("disable display\n");
				ret = regulator_disable(disp_ldsw);
				printk("disable display ldo ret %d\n", ret);
			} else {
				printk("enable display\n");
				ret = regulator_enable(disp_ldsw);
				k_msleep(50);
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
