
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdbool.h>
#include "config.h"

bool ble_is_advertising();
bool ble_is_connected();

bool is_waiting_for_passkey_confirmation();
void confirm_passkey();
void reject_passkey();

int init_bluetooth();
void send_encoded_keys(struct encoded_keys keys);


#endif // BLUETOOTH_H