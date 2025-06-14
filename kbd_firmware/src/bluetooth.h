
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdbool.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "config.h"

bool ble_is_advertising();
bool ble_is_connected();

bool is_waiting_for_passkey_confirmation();
void confirm_passkey();
void reject_passkey();

int init_bluetooth();
void send_encoded_keys(struct encoded_keys keys);

struct addr {
  char addr[BT_ADDR_LE_STR_LEN];
};

struct addr get_current_addr();

void send_bas_soc(float soc);

#endif  // BLUETOOTH_H
