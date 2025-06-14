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

extern struct pmic_state pmic_state;

void update_pmic_state();

void enter_ship_mode();

void init_pmic();

#endif  // PMIC_H
