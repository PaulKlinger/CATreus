#ifndef PMIC_H
#define PMIC_H

#include <stdbool.h>
#include <stdint.h>

struct pmic_state {
  int32_t charger_status;
  int32_t charger_error;
  bool is_charging;
  bool vbus_present;
  float battery_voltage;
  float battery_current;
  float temp;
};

struct pmic_state get_pmic_state();

void enter_ship_mode();

#endif  // PMIC_H
