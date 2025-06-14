#include "fuel_gauge.h"

#include <nrf_fuel_gauge.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "../pmic.h"

K_THREAD_STACK_DEFINE(fuel_gauge_thread_stack, 2048);
struct k_thread fuel_gauge_thread_data;

/* nPM1300 CHARGER.BCHGCHARGESTATUS register bitmasks */
#define NPM1300_CHG_STATUS_COMPLETE_MASK BIT(1)
#define NPM1300_CHG_STATUS_TRICKLE_MASK BIT(2)
#define NPM1300_CHG_STATUS_CC_MASK BIT(3)
#define NPM1300_CHG_STATUS_CV_MASK BIT(4)

static int64_t ref_time;

static const struct battery_model battery_model = {
#include "14500s800mah_25C.inc"
};

static const struct device *charger =
    DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));

struct battery_state battery_state;

static int charge_status_inform(int32_t chg_status) {
  union nrf_fuel_gauge_ext_state_info_data state_info;

  if (chg_status & NPM1300_CHG_STATUS_COMPLETE_MASK) {
    printk("Charge complete\n");
    state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;
  } else if (chg_status & NPM1300_CHG_STATUS_TRICKLE_MASK) {
    printk("Trickle charging\n");
    state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
  } else if (chg_status & NPM1300_CHG_STATUS_CC_MASK) {
    printk("Constant current charging\n");
    state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC;
  } else if (chg_status & NPM1300_CHG_STATUS_CV_MASK) {
    printk("Constant voltage charging\n");
    state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
  } else {
    printk("Charger idle\n");
    state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
  }

  return nrf_fuel_gauge_ext_state_update(
      NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE, &state_info);
}

int fuel_gauge_update() {
  static int32_t chg_status_prev;

  float soc;
  float tte;
  float ttf;
  float delta;
  int ret;

  update_pmic_state();

  ret = nrf_fuel_gauge_ext_state_update(
      pmic_state.vbus_present ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
                              : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
      NULL);
  if (ret < 0) {
    printk("Error: Could not inform of state\n");
    return ret;
  }

  if (pmic_state.charger_status != chg_status_prev) {
    chg_status_prev = pmic_state.charger_status;

    ret = charge_status_inform(pmic_state.charger_status);
    if (ret < 0) {
      printk("Error: Could not inform of charge status\n");
      return ret;
    }
  }

  delta = (float)k_uptime_delta(&ref_time) / 1000.f;

  soc = nrf_fuel_gauge_process(pmic_state.battery_voltage,
                               pmic_state.battery_current, pmic_state.temp,
                               delta, NULL);
  tte = nrf_fuel_gauge_tte_get();
  ttf = nrf_fuel_gauge_ttf_get();

  printk("SoC: %.2f%%, TTE: %.0f, TTF: %.0f\n", (double)soc, (double)tte,
         (double)ttf);

  battery_state.soc = soc;
  battery_state.tte_s = tte;
  battery_state.ttf_s = ttf;

  return 0;
}

void fuel_gauge_loop() {
  while (true) {
    k_msleep(4000);
    fuel_gauge_update();
  }
}

int init_fuel_gauge() {
  battery_state = (struct battery_state){0};
  struct sensor_value value;
  struct nrf_fuel_gauge_init_parameters parameters = {
      .model = &battery_model,
      .opt_params = NULL,
      .state = NULL,
  };
  float max_charge_current;
  float term_charge_current;
  int ret;

  printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

  parameters.v0 = pmic_state.battery_voltage;
  parameters.i0 = pmic_state.battery_current;
  parameters.t0 = pmic_state.temp;

  /* Store charge nominal and termination current, needed for ttf calculation
   */
  sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT,
                     &value);
  max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
  term_charge_current = max_charge_current / 10.f;

  ret = nrf_fuel_gauge_init(&parameters, NULL);
  if (ret < 0) {
    printk("Error: Could not initialise fuel gauge\n");
    return ret;
  }

  ret = nrf_fuel_gauge_ext_state_update(
      NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
      &(union nrf_fuel_gauge_ext_state_info_data){.charge_current_limit =
                                                      max_charge_current});
  if (ret < 0) {
    printk("Error: Could not set fuel gauge state\n");
    return ret;
  }

  ret = nrf_fuel_gauge_ext_state_update(
      NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
      &(union nrf_fuel_gauge_ext_state_info_data){.charge_term_current =
                                                      term_charge_current});
  if (ret < 0) {
    printk("Error: Could not set fuel gauge state\n");
    return ret;
  }

  ret = charge_status_inform(pmic_state.charger_status);
  if (ret < 0) {
    printk("Error: Could not set fuel gauge state\n");
    return ret;
  }

  ref_time = k_uptime_get();

  k_thread_create(&fuel_gauge_thread_data, fuel_gauge_thread_stack,
                  K_THREAD_STACK_SIZEOF(fuel_gauge_thread_stack),
                  fuel_gauge_loop, NULL, NULL, NULL, K_PRIO_PREEMPT(6), 0,
                  K_NO_WAIT);

  return 0;
}
