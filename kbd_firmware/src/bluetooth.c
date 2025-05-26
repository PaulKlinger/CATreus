#include "bluetooth.h"
#include "config.h"
#include "ui.h"
#include "leds.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <assert.h>
#include <zephyr/spinlock.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/bluetooth/services/bas.h>
#include <bluetooth/services/hids.h>
#include <zephyr/bluetooth/services/dis.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static volatile bool is_adv;
static volatile bool waiting_for_passkey_confirmation = false;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_conn *current_conn = NULL;

static struct k_work adv_work;

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void adv_work_handler(struct k_work *work)
{
    int err;
    const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
        BT_LE_ADV_OPT_CONN,
        BT_GAP_ADV_FAST_INT_MIN_2,
        BT_GAP_ADV_FAST_INT_MAX_2,
        NULL);

    err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
                          ARRAY_SIZE(sd));
    if (err)
    {
        if (err == -EALREADY)
        {
            printk("Advertising continued\n");
        }
        else
        {
            printk("Advertising failed to start (err %d)\n", err);
        }

        return;
    }

    is_adv = true;
    led_start_advertising_anim();
    printk("Advertising successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err)
    {
        printk("Failed to connect to %s 0x%02x %s\n", addr, err, bt_hci_err_to_str(err));
        return;
    }
    current_conn = bt_conn_ref(conn);

    printk("Connected %s\n", addr);
    is_adv = false;
    led_stop_anim();

    if (bt_conn_set_security(conn, BT_SECURITY_L3)) {
		printk("Failed to set security\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));
    bt_conn_unref(current_conn);
    current_conn = NULL;
    advertising_start();
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        printk("Security changed: %s level %u\n", addr, level);
    }
    else
    {
        printk("Security failed: %s level %u err %d %s\n", addr, level, err,
               bt_security_err_to_str(err));
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    if (conn != current_conn)
    {
        if (current_conn) {bt_conn_unref(current_conn);}
        current_conn = bt_conn_ref(conn);
    }
    ui_send_display_passkey(passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    if (conn != current_conn)
    {
        if (current_conn) {bt_conn_unref(current_conn);}
        current_conn = bt_conn_ref(conn);
    }
    waiting_for_passkey_confirmation = true;
    ui_send_confirm_passkey(passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
    if (current_conn)
    {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
           bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed};

static void num_comp_reply(bool accept)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(current_conn), addr, sizeof(addr));
    printk("num_comp_reply(%d), conn: %s\n", accept, addr);
    if (accept)
    {
        bt_conn_auth_passkey_confirm(current_conn);
        printk("Numeric Match, conn %p\n", current_conn);
    }
    else
    {
        bt_conn_auth_cancel(current_conn);
        printk("Numeric Reject, conn %p\n", current_conn);
        bt_conn_unref(current_conn);
    }

}

void confirm_passkey(void)
{
    num_comp_reply(true);
    waiting_for_passkey_confirmation = false;
}

void reject_passkey(void)
{
    num_comp_reply(false);
    waiting_for_passkey_confirmation = false;
}

bool ble_is_advertising()
{
    return is_adv;
}

bool ble_is_connected()
{
    return current_conn != NULL;
}

bool is_waiting_for_passkey_confirmation()
{
    return waiting_for_passkey_confirmation;
}

int init_bluetooth(void)
{
    waiting_for_passkey_confirmation = false;
    int err;

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err)
    {
        printk("Failed to register authorization callbacks.\n");
        return err;
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err)
    {
        printk("Failed to register authorization info callbacks.\n");
        return err;
    }

    err = bt_enable(NULL);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    printk("Bluetooth initialized\n");

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    
	k_work_init(&adv_work, adv_work_handler);
    advertising_start();
    return 0;
}