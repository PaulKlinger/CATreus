/** @file
 *  @brief HID Service
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "key_layout.h"


#define INPUT_REP_KEYS_REF_ID 0
#define OUTPUT_REP_KEYS_REF_ID 0

enum
{
    HIDS_REMOTE_WAKE = BIT(0),
    HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info
{
    uint16_t version; /* version number of base USB HID Specification */
    uint8_t code;     /* country HID Device hardware is localized for. */
    uint8_t flags;
} __packed;

struct hids_report
{
    uint8_t id;   /* report id */
    uint8_t type; /* report type */
} __packed;

static struct hids_info info = {
    .version = 0x0101,
    .code = 0x00,
    .flags = HIDS_NORMALLY_CONNECTABLE,
};

enum
{
    HIDS_INPUT = 0x01,
    HIDS_OUTPUT = 0x02,
    HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
    .id = INPUT_REP_KEYS_REF_ID,
    .type = HIDS_INPUT,
};

static struct hids_report output = {
    .id = OUTPUT_REP_KEYS_REF_ID,
    .type = HIDS_OUTPUT,
};

static uint8_t simulate_input;
static uint8_t ctrl_point;
static uint8_t report_map[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)

    /* keys */
#if INPUT_REP_KEYS_REF_ID
		0x85, INPUT_REP_KEYS_REF_ID,
#endif
		0x05, 0x07,       /* Usage Page (Key Codes) */
		0x19, 0xe0,       /* Usage Minimum (224) */
		0x29, 0xe7,       /* Usage Maximum (231) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x08,       /* Report Count (8) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */

		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x08,       /* Report Size (8) */
		0x81, 0x01,       /* Input (Constant) reserved byte(1) */

		0x95, 0x06,       /* Report Count (6) */
		0x75, 0x08,       /* Report Size (8) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x65,       /* Logical Maximum (101) */
		0x05, 0x07,       /* Usage Page (Key codes) */
		0x19, 0x00,       /* Usage Minimum (0) */
		0x29, 0x65,       /* Usage Maximum (101) */
		0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */

		/* LED */
#if OUTPUT_REP_KEYS_REF_ID
		0x85, OUTPUT_REP_KEYS_REF_ID,
#endif
		0x95, 0x05,       /* Report Count (5) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x08,       /* Usage Page (Page# for LEDs) */
		0x19, 0x01,       /* Usage Minimum (1) */
		0x29, 0x05,       /* Usage Maximum (5) */
		0x91, 0x02,       /* Output (Data, Variable, Absolute), */
				  /* Led report */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x91, 0x01,       /* Output (Data, Variable, Absolute), */
				  /* Led report padding */

		0xC0              /* End Collection (Application) */
};

static ssize_t read_info(struct bt_conn *conn,
                         const struct bt_gatt_attr *attr, void *buf,
                         uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
                             sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr, void *buf,
                               uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
                             sizeof(report_map));
}

static ssize_t read_report(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
                             sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    simulate_input = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_input_report(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset,
                                uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(ctrl_point))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

#define SAMPLE_BT_PERM_READ BT_GATT_PERM_READ_ENCRYPT
#define SAMPLE_BT_PERM_WRITE BT_GATT_PERM_WRITE_ENCRYPT

/* HID Service Declaration */
// BT_GATT_SERVICE_DEFINE(
//     hid_svc,
//     BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
//     BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
//                            BT_GATT_PERM_READ, read_info, NULL, &info),
//     BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
//                            BT_GATT_PERM_READ, read_report_map, NULL, NULL),
//     BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
//                            BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
//                            SAMPLE_BT_PERM_READ,
//                            read_input_report, NULL, NULL),
//     BT_GATT_CCC(input_ccc_changed,
//                 SAMPLE_BT_PERM_READ | SAMPLE_BT_PERM_WRITE),
//     BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
//                        read_report, NULL, &input),
//     BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
//                            BT_GATT_CHRC_WRITE_WITHOUT_RESP,
//                            BT_GATT_PERM_WRITE,
//                            NULL, write_ctrl_point, &ctrl_point), );

static uint8_t protocol_mode = 1; // default to protocol mode

static uint8_t output_report = 0;

static ssize_t read_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                                  uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(protocol_mode));
}

static ssize_t write_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(protocol_mode))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static ssize_t read_output_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                                  uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data, sizeof(output_report));
}

static ssize_t write_output_report(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t *value = attr->user_data;

    if (offset + len > sizeof(protocol_mode))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    return len;
}

static void boot_input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
}

BT_GATT_SERVICE_DEFINE(
    hid_keyboard_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_protocol_mode,
                           write_protocol_mode, &protocol_mode),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_input_report, NULL, NULL),
    BT_GATT_CCC(input_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report,
                       NULL, &input),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_output_report,
                           write_output_report, &output_report),
    BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ, read_report,
                       NULL, &output),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
                           read_report_map, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_IN_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, read_input_report, NULL, NULL),
    BT_GATT_CCC(boot_input_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_OUT_REPORT,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_output_report,
                           write_output_report, &output_report),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_info,
                           NULL, &info),
    BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, write_ctrl_point, &ctrl_point));

void hid_init(void)
{
}

void send_encoded_keys(struct encoded_keys keys)
{
    /* HID Report:
     * Byte 0: modifier mask
     * Byte 1: reserved (must be 0)
     * Byte 2-7: keys (up to 6 keys)
     */
    uint8_t report[8] = {0};

    report[0] = keys.modifier_mask;
    report[1] = 0; // reserved byte, always 0

    for (int i = 0; i < MAX_N_ENCODED_KEYS; i++)
    {
        report[i + 2] = keys.keys[i];
    }
    int ret = bt_gatt_notify_uuid(NULL, BT_UUID_HIDS_REPORT, hid_keyboard_service.attrs, report, sizeof(report));
    printk("HID report sent, ret: %d\n", ret);
}
