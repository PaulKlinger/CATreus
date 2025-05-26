#include "ui.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include "bluetooth.h"
#include "config.h"
#include "display.h"
#include "pmic.h"

#define THREAD_STACK_SIZE 1024
#define PRIORITY 5
#define UI_TIME_STEP_MS 100
#define UI_TIMEOUT_MS 10000

// ----------------------------------------------------
// Thread & queue defs

K_THREAD_STACK_DEFINE(ui_thread_stack, THREAD_STACK_SIZE);
struct k_thread ui_thread_data;
k_tid_t ui_thread_id;

struct ui_message {
    enum ui_message_type {
        UI_MESSAGE_TYPE_STARTUP,
        UI_MESSAGE_TYPE_WAKE_PRESSED,
        UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED,
        UI_MESSAGE_TYPE_KEY_PRESSED,  // only sent if ui is active
        UI_MESSAGE_TYPE_CONFIRM_PASSKEY,
        UI_MESSAGE_TYPE_DISPLAY_PASSKEY,
        UI_MESSAGE_TYPE_PAIRING,
        // to indicate a page is not triggered by a message
        UI_MESSAGE_TYPE_NOMSG,
    } type;
    union {
        struct key_coord key;
        unsigned int passkey;
    } data;
};

char ui_message_buffer[10 * sizeof(struct ui_message)];
struct k_msgq ui_messages;

// ----------------------------------------------------
// functions to send to queue

void send_ui_message(struct ui_message msg) {
    int ret = k_msgq_put(&ui_messages, &msg, K_NO_WAIT);
    if (ret != 0) {
        printk("Error %d: failed to send UI message\n", ret);
    } else {
        printk("UI message sent: type %d\n", msg.type);
    }
}

void ui_send_wake_and_key(struct key_coord key) {
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED;
    msg.data.key = key;
    send_ui_message(msg);
}

void ui_send_key(struct key_coord key) {
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_KEY_PRESSED;
    msg.data.key = key;
    send_ui_message(msg);
}

void ui_send_startup() {
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_STARTUP;
    send_ui_message(msg);
}

void ui_send_wake() {
    printk("sending wake message\n");
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_WAKE_PRESSED;
    send_ui_message(msg);
}

void ui_send_confirm_passkey(unsigned int passkey) {
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_CONFIRM_PASSKEY;
    msg.data.passkey = passkey;
    send_ui_message(msg);
}

void ui_send_display_passkey(unsigned int passkey) {
    struct ui_message msg;
    msg.type = UI_MESSAGE_TYPE_DISPLAY_PASSKEY;
    msg.data.passkey = passkey;
    send_ui_message(msg);
}

// state definitions

enum ui_page {
    UI_DISABLED = 0,
    UI_PAGE_STARTUP = 1,
    UI_PAGE_SHUTDOWN = 2,
    UI_PAGE_DEBUG = 3,
    UI_PAGE_CONFIRM_PASSKEY = 4,
    UI_PAGE_DISPLAY_PASSKEY = 5,
    UI_PAGE_WAKEUP = 6,
    __UI_N_PAGES = 7,
};
union ui_page_state {
    uint32_t frame_idx;
    unsigned int passkey;
};

struct ui_state {
    enum ui_page current_page;
    union ui_page_state page_state;
    int64_t last_msg_time;
};

void open_page(struct ui_state *state, enum ui_page new_page) {
    printk("switching page to %d", new_page);
    state->current_page = new_page;
    state->page_state = (union ui_page_state){0};
}

// ----------------------------------------------------
// page implementations
// TODO: move to separate files

void show_debug_page(struct ui_message msg, struct ui_state *state) {
    struct pmic_state pmic_state = get_pmic_state();
    int64_t uptime = k_uptime_get();

    char str[80];
    sprintf(str,
            "usb %d s %d e %d \n %1.0fmA %1.3fV\nconn: %d\nuptime: %4lldm %2llds ",
            pmic_state.vbus_present, pmic_state.charger_status,
            pmic_state.charger_error, (double)pmic_state.battery_current,
            (double)pmic_state.battery_voltage, ble_is_connected(),
            uptime / 60000, uptime % 60000 / 1000);

    lcd_goto_xpix_y(0, 0);
    lcd_clear_buffer();
    lcd_puts(str);
    lcd_display();
}

void show_confirm_passkey_page(struct ui_message msg, struct ui_state *state) {
    if (msg.type == UI_MESSAGE_TYPE_CONFIRM_PASSKEY) {
        state->page_state.passkey = msg.data.passkey;
    }
    char str[60];

    lcd_clear_buffer();

    if (is_waiting_for_passkey_confirmation()) {
        if (msg.type == UI_MESSAGE_TYPE_KEY_PRESSED && msg.data.key.row == 2 &&
            msg.data.key.col == 2) {
            // Confirm passkey
            confirm_passkey();
        } else if (msg.type == UI_MESSAGE_TYPE_KEY_PRESSED &&
                   msg.data.key.row == 1 && msg.data.key.col == 7) {
            // Reject passkey
            reject_passkey();
        }
        sprintf(str, "pairing request\nkey: %06u\n\npress y/n",
                state->page_state.passkey);
        lcd_goto_xpix_y(0, 2);
        lcd_puts(str);
        lcd_display();
    } else {
        // not too sure here, this means one UI cycle delay
        // sending a message instead would be immediate, but semantically
        // dubious
        open_page(state, UI_PAGE_WAKEUP);
    }
}

void show_display_passkey_page(struct ui_message msg, struct ui_state *state) {
    unsigned int passkey = msg.data.passkey;
    lcd_goto_xpix_y(0, 2);
    lcd_clear_buffer();
    char str[30];
    sprintf(str, "pairing request\nkey: %06u", passkey);
    lcd_puts(str);
    lcd_display();
}

void show_startup_page(struct ui_message msg, struct ui_state *state) {
    lcd_goto_xpix_y(0, 3);
    lcd_clear_buffer();
    lcd_puts("Starting...");
    lcd_display();
}

void show_shutdown_page(struct ui_message msg, struct ui_state *state) {
    lcd_goto_xpix_y(0, 3);
    lcd_clear_buffer();
    lcd_puts("Shutting down...");
    lcd_display();
    k_msleep(500);
    enter_ship_mode();
}

void show_wakeup_page(struct ui_message msg, struct ui_state *state) {
    lcd_goto_xpix_y(50, 3);
    lcd_clear_buffer();
    lcd_puts("^ _ ^");
    lcd_display();
}

struct ui_page_cfg {
    void (*show)(struct ui_message, struct ui_state *state);
    enum ui_message_type
        trigger_msg;               // type of message that triggers this page
    struct key_coord trigger_key;  // press wake + trigger_key to show this page
    bool allow_navigation;  // if true, respond to triggers of other pages
};
struct ui_page_cfg ui_page_cfgs[__UI_N_PAGES];

#define NO_KEY \
    {42, 42}   \
    // No key defined, used for pages that don't require a key press to show
void init_ui_page_cfg() {
    ui_page_cfgs[UI_DISABLED] =
        (struct ui_page_cfg){NULL, UI_MESSAGE_TYPE_NOMSG, NO_KEY, true};
    ui_page_cfgs[UI_PAGE_STARTUP] = (struct ui_page_cfg){
        show_startup_page, UI_MESSAGE_TYPE_STARTUP, NO_KEY, false};
    ui_page_cfgs[UI_PAGE_SHUTDOWN] =
        (struct ui_page_cfg){show_shutdown_page,
                             UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED,
                             {1, 6},
                             false};
    ui_page_cfgs[UI_PAGE_DEBUG] = (struct ui_page_cfg){
        show_debug_page, UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED, {1, 10}, true};
    ui_page_cfgs[UI_PAGE_CONFIRM_PASSKEY] =
        (struct ui_page_cfg){show_confirm_passkey_page,
                             UI_MESSAGE_TYPE_CONFIRM_PASSKEY, NO_KEY, false};
    ui_page_cfgs[UI_PAGE_DISPLAY_PASSKEY] =
        (struct ui_page_cfg){show_display_passkey_page,
                             UI_MESSAGE_TYPE_DISPLAY_PASSKEY, NO_KEY, true};
    ui_page_cfgs[UI_PAGE_WAKEUP] = (struct ui_page_cfg){
        show_wakeup_page, UI_MESSAGE_TYPE_WAKE_PRESSED, NO_KEY, true};
};

void switch_page(struct ui_state *state, struct ui_message *msg) {
    for (int i = 0; i < __UI_N_PAGES; i++) {
        struct ui_page_cfg cfg = ui_page_cfgs[i];
        if (cfg.trigger_msg == msg->type &&
            ((cfg.trigger_key.row == 42 && cfg.trigger_key.col == 42) ||
             (cfg.trigger_key.row == msg->data.key.row &&
              cfg.trigger_key.col == msg->data.key.col))) {
            if (state->current_page != i) {
                open_page(state, (enum ui_page)i);
            }
            break;
        }
    }
}

static struct ui_state current_ui_state;

bool ui_active() { return current_ui_state.current_page != UI_DISABLED; }

void ui_thread() {
    current_ui_state = (struct ui_state){
        .current_page = UI_DISABLED,
        .page_state = {0},
        .last_msg_time = k_uptime_get(),
    };
    struct ui_state *state = &current_ui_state;
    struct ui_message msg;

    int ret;
    while (1) {
        printk("ui loop\n");
        if (state->current_page != UI_DISABLED) {
            // if the display is active update every UI_TIMESTEMP_MS
            ret = k_msgq_get(&ui_messages, &msg, K_MSEC(UI_TIME_STEP_MS));
        } else {
            // else sleep until we get a ui message
            ret = k_msgq_get(&ui_messages, &msg, K_FOREVER);
        }
        if (ret == 0) {
            printk("Got ui message, type: %d\n", msg.type);
            // got a message, make sure display is on
            if (state->current_page == UI_DISABLED) {
                display_init();
            }
            printk("setting uptime");
            state->last_msg_time = k_uptime_get();

            if (ui_page_cfgs[state->current_page].allow_navigation) {
                switch_page(state, &msg);
            }
        }
        if (k_uptime_get() - state->last_msg_time > UI_TIMEOUT_MS) {
            disable_display();
            // TODO: ui sleep page?
            state->current_page = UI_DISABLED;
            continue;
        }
        if (state->current_page != UI_DISABLED) {
            ui_page_cfgs[state->current_page].show(msg, state);
        }
    }
}

void suspend_ui() {
    k_thread_suspend(ui_thread_id);
}

void resume_ui() {
    k_thread_resume(ui_thread_id);
}

void init_ui(void) {
    k_msgq_init(&ui_messages, ui_message_buffer, sizeof(struct ui_message), 10);
    disable_display();
    init_ui_page_cfg();
    ui_thread_id = k_thread_create(&ui_thread_data, ui_thread_stack,
                    K_THREAD_STACK_SIZEOF(ui_thread_stack), ui_thread, NULL,
                    NULL, NULL, PRIORITY, 0, K_NO_WAIT);
};