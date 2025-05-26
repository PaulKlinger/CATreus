#include "pmic.h"

#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/kernel.h>

#include "key_matrix.h"

static const struct device *charger =
    DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));


static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));


int get_charger_attr(enum sensor_channel channel, enum sensor_attribute attr,
                     struct sensor_value *val) {
    int ret = sensor_attr_get(charger, channel, attr, val);
    if (ret) {
        printk("charger attr get failed %d\n", ret);
    }
    return ret;
}

int get_charger_channel(enum sensor_channel channel, struct sensor_value *val) {
    int ret = sensor_channel_get(charger, channel, val);
    if (ret) {
        printk("charger channel get failed %d\n", ret);
    }
    return ret;
}

struct pmic_state get_pmic_state() {
    struct pmic_state state = {0};

    struct sensor_value val;

    sensor_sample_fetch(charger);
    get_charger_attr(SENSOR_CHAN_NPM1300_CHARGER_VBUS_STATUS,
                     SENSOR_ATTR_NPM1300_CHARGER_VBUS_PRESENT, &val);
    state.vbus_present = val.val1;

    get_charger_channel(SENSOR_CHAN_NPM1300_CHARGER_STATUS, &val);
    state.charger_status = val.val1;
    state.is_charging = (state.charger_status & 0b00011000) !=
                        0;  // constant current or constant voltage

    get_charger_channel(SENSOR_CHAN_GAUGE_VOLTAGE, &val);
    state.battery_voltage = (float)val.val1 + ((float)val.val2) / 1000000.0f;

    get_charger_channel(SENSOR_CHAN_GAUGE_AVG_CURRENT, &val);
    state.battery_current =
        (float)val.val1 * 1000.f + ((float)val.val2) / 1000.0f;

    return state;
}

void enter_ship_mode() {
    while (wake_pressed()) {
        k_msleep(10);
    }
    k_msleep(100);
    printk("Going to sleep (ship mode)\n");
    int ret = regulator_parent_ship_mode(regulators);
    printk("ship mode ret %d\n", ret);
    printk("Should be asleep!\n");
    k_msleep(100);
    printk("Should have been asleep 100ms!\n");
    k_sleep(K_FOREVER);
}