#ifndef FUEL_GAUGE_H
#define FUEL_GAUGE_H

int init_fuel_gauge();

struct battery_state {
    float soc, tte_s, ttf_s;
};

extern struct battery_state battery_state;

#endif /* __FUEL_GAUGE_H__ */