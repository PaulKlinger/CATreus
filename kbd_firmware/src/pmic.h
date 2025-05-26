#ifndef PMIC_H
#define PMIC_H

#include <stdbool.h>

struct pmic_state {
    int charger_status;
    int charger_error;
    bool is_charging;
    bool vbus_present;
    float battery_voltage;
    float battery_current;
};

struct pmic_state get_pmic_state();

void enter_ship_mode();

#endif // PMIC_H