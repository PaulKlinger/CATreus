#ifndef NVS_H
#define NVS_H
#include <stdbool.h>
#include <stdint.h>

int nvs_init();

bool nvs_get_ctrl_cmd_config();
void nvs_store_ctrl_cmd(bool swap_ctrl_cmd);

#endif  // NVS_H
