#include "ui.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>

#include <zephyr/drivers/i2c.h>

#include "display.h"

static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));

int get_charger_attr(enum sensor_channel channel, enum sensor_attribute attr, struct sensor_value *val)
{
    int ret = sensor_attr_get(charger, channel, attr, val);
    if (ret)
    {
        printk("charger attr get failed %d\n", ret);
    }
    return ret;
}

int get_charger_channel(enum sensor_channel channel, struct sensor_value *val)
{
    int ret = sensor_channel_get(charger, channel, val);
    if (ret)
    {
        printk("charger channel get failed %d\n", ret);
    }
    return ret;
}

void show_debug_page()
{

    struct sensor_value val;
    sensor_sample_fetch(charger);
    get_charger_attr(SENSOR_CHAN_NPM1300_CHARGER_VBUS_STATUS, SENSOR_ATTR_NPM1300_CHARGER_VBUS_PRESENT, &val);
    int32_t vbus_present = val.val1;
    get_charger_channel(SENSOR_CHAN_NPM1300_CHARGER_STATUS, &val);
    int32_t charger_status = val.val1;
    get_charger_channel(SENSOR_CHAN_NPM1300_CHARGER_ERROR, &val);
    int32_t charger_error = val.val1;
    get_charger_channel(SENSOR_CHAN_GAUGE_AVG_CURRENT, &val);
    char current[6];
    gcvt((float)val.val1 * 1000.f + ((float)val.val2) / 1000.0f, 4, current);
    get_charger_channel(SENSOR_CHAN_GAUGE_VOLTAGE, &val);
    char voltage[6];
    gcvt((float)val.val1 + ((float)val.val2) / 1000000.0f, 4, voltage);
    char str[100];
    sprintf(str, "usb %d s %d e %d \n %smA %sV \n", vbus_present, charger_status, charger_error, current, voltage);
    printk("%s", str);

    lcd_goto_xpix_y(0, 0);
    lcd_clear_buffer();
    lcd_puts(str);
    lcd_display();
}

void confirm_passkey_dialog(unsigned int passkey) {
    if (!display_enabled()) {
        display_init();
    }
    lcd_goto_xpix_y(0, 2);
    lcd_clear_buffer();
    char str[60];
    sprintf(str, "pairing request\nkey: %06u\npress y/n", passkey);
    lcd_puts(str);
    lcd_display();
}


void display_passkey_dialog(unsigned int passkey) {
    if (!display_enabled()) {
        display_init();
    }
    lcd_goto_xpix_y(0, 2);
    lcd_clear_buffer();
    char str[30];
    sprintf(str, "pairing request\nkey: %06u", passkey);
    lcd_puts(str);
    lcd_display();
}