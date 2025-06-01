#ifndef NVS_H
#define NVS_H
#include <stdbool.h>
#include <stdint.h>

void nvs_store_n_boot(uint16_t value);
uint16_t nvs_read_n_boot();
int nvs_init();

bool nvs_get_ctrl_cmd_config();
void nvs_store_ctrl_cmd(bool swap_ctrl_cmd);

#endif  // NVS_H
