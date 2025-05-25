
#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdbool.h>

bool is_advertising();

bool is_waiting_for_passkey_confirmation();
void confirm_passkey();
void reject_passkey();

int init_bluetooth(void);

#endif // BLUETOOTH_H