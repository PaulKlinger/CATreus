#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include "config.h"

bool in_application();

void ui_send_wake_and_key(struct key_coord key);

void ui_send_key(struct key_coord key);

void ui_send_startup();

void ui_send_wake();

void ui_send_confirm_passkey(unsigned int passkey);

void ui_send_display_passkey(unsigned int passkey);

bool ui_active();

void init_ui();

void suspend_ui();

void resume_ui();

#endif // UI_H