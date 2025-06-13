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

#include "animations/anim.h"
#include "animations/anim_idle.h"
#include "animations/anim_sleep.h"
#include "animations/anim_wake.h"
#include "applications/breakout.h"
#include "applications/gol.h"
#include "applications/lander.h"
#include "applications/mandelbrot.h"
#include "applications/mines.h"
#include "applications/snake.h"
#include "applications/tetris.h"
#include "bluetooth.h"
#include "config.h"
#include "display.h"
#include "fuel_gauge/fuel_gauge.h"
#include "key_layout.h"
#include "key_matrix.h"
#include "nvs.h"
#include "pmic.h"

#define THREAD_STACK_SIZE 1024
#define PRIORITY 5
#define UI_TIME_STEP_MS 50
#define UI_TIMEOUT_MS 10000

// Whether an application (exclusive use of keyboard) is running
bool application_running = false;

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

#define UI_MSG_QUEUE_SIZE 1
char ui_message_buffer[UI_MSG_QUEUE_SIZE * sizeof(struct ui_message)];
struct k_msgq ui_messages;

// ----------------------------------------------------
// functions to send to queue

void send_ui_message(struct ui_message msg) {
    int ret = k_msgq_put(&ui_messages, &msg, K_MSEC(UI_TIME_STEP_MS));
    if (ret != 0) {
        printk("Error %d: failed to send UI message\n", ret);
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
    UI_PAGE_SWAP_CTRL_CMD = 6,
    UI_PAGE_HELP = 7,
    UI_PAGE_APPS = 8,
    UI_PAGE_IDLE = 9,
    __UI_N_PAGES = 10,
};
struct anim_state {
    uint32_t frame_idx;
    int64_t frame_start_time;
};
union ui_page_state {
    uint32_t idx;  // e.g. menu index, etc.
    struct anim_state anim;
    unsigned int passkey;
};

struct ui_state {
    enum ui_page current_page;
    union ui_page_state page_state;
    int64_t last_msg_time;
};

void open_page(struct ui_state *state, enum ui_page new_page) {
    state->current_page = new_page;
    state->page_state = (union ui_page_state){0};
}

bool show_animation(struct anim_state *state, struct animation *anim, bool show_soc) {
    if (state->frame_start_time == 0) {
        state->frame_start_time = k_uptime_get();
        state->frame_idx = anim->init_idx;
    }
    memcpy(displayBuffer, anim->frames[state->frame_idx],
           sizeof(displayBuffer));
    if (show_soc) {
      char soc_str[6];
      sprintf(soc_str, "%.0f%%", (double)battery_state.soc);
      lcd_goto_xpix_y(110, 0);
      lcd_puts_invert(soc_str);
    }
    lcd_display();
    if (k_uptime_get() - state->frame_start_time >
        100 * (int)anim->frame_counts[state->frame_idx]) {
        if (state->frame_idx == anim->end_idx) {
            if (anim->loop) {
                state->frame_idx = anim->start_idx;
                state->frame_start_time = k_uptime_get();
            } else {
                // animation finished
                return false;
            }
        } else {
            state->frame_idx += anim->frame_step;
            state->frame_start_time = k_uptime_get();
        }
    }
    return true;
}

// ----------------------------------------------------
// page implementations
// TODO: move to separate files

void show_debug_page(struct ui_message msg, struct ui_state *state) {
    struct pmic_state pmic_state = get_pmic_state();
    int64_t uptime = k_uptime_get();
    char str[120];
    sprintf(
        str,
        "usb %d s %d e %d \n %1.0fmA %1.3fV conn: %d\nuptime: %4lldm "
        "%2llds\nks: %d wake: %d\nswap: %d\nsoc: %.2f%%\ntte: %.1fh ttf: %.0fm",
        pmic_state.vbus_present, pmic_state.charger_status,
        pmic_state.charger_error, (double)pmic_state.battery_current,
        (double)pmic_state.battery_voltage, ble_is_connected(), uptime / 60000,
        uptime % 60000 / 1000, current_pressed_keys.n_pressed,
        current_pressed_keys.wake_pressed, ctrl_cmd_swapped, (double)battery_state.soc,
        (double)(battery_state.tte_s / 60.f / 60.f),
        (double)(battery_state.ttf_s / 60.f));

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
        open_page(state, UI_PAGE_IDLE);
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

void show_shutdown_page(struct ui_message msg, struct ui_state *state) {
    while (show_animation(&state->page_state.anim, &anim_sleep, false)) {
        k_msleep(UI_TIME_STEP_MS);
    }
    enter_ship_mode();
}

void show_startup_page(struct ui_message msg, struct ui_state *state) {
    bool anim_running = show_animation(&state->page_state.anim, &anim_wake, false);
    if (!anim_running) {
        open_page(state, UI_PAGE_IDLE);
    }
}

void show_swap_ctrl_cmd_page(struct ui_message msg, struct ui_state *state) {
    lcd_goto_xpix_y(0, 3);
    if (state->page_state.idx == 0) {
        state->page_state.idx = 1;
        lcd_clear_buffer();
        lcd_goto_xpix_y(15, 3);
        swap_ctrl_cmd();
        // TODO: save this per connection to flash
        if (ctrl_cmd_swapped) {
            lcd_puts("[ctrl]     [cmd]");
        } else {
            lcd_puts("[cmd]     [ctrl]");
        }
    }
    lcd_display();
    k_msleep(350);
    open_page(state, UI_PAGE_IDLE);
}

void show_help_page(struct ui_message msg, struct ui_state *state) {
    lcd_goto_xpix_y(0, 0);
    lcd_clear_buffer();
    lcd_puts("wake + <key>\n");
    lcd_puts("H: this page\n");
    lcd_puts("S: shutdown\n");
    lcd_puts("D: debug info\n");
    lcd_puts("W: swap ctrl & cmd\n");
    lcd_puts("A: apps menu\n");
    lcd_display();
}

void run_application(void (*app_func)(void)) {
    application_running = true;
    app_func();
    application_running = false;
}

void show_apps_page(struct ui_message msg, struct ui_state *state) {
    if (msg.type == UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED) {
        if (keq(msg.data.key, (struct key_coord){2, 6})) {
            // Start breakout app
            run_application(run_breakout);
        } else if (keq(msg.data.key, (struct key_coord){0, 8})) {
            // Start gol app
            run_application(run_gol);
        } else if (keq(msg.data.key, (struct key_coord){0, 2})) {
            // Start lander app
            run_application(run_lander);
        } else if (keq(msg.data.key, (struct key_coord){2, 7})) {
            // Start mandelbrot app
            run_application(run_mandelbrot);
        } else if (keq(msg.data.key, (struct key_coord){1, 1})) {
            // Start mines app
            run_application(run_mines);
        } else if (keq(msg.data.key, (struct key_coord){1, 6})) {
            // Start snake app
            run_application(run_snake);
        } else if (keq(msg.data.key, (struct key_coord){1, 9})) {
            // Start tetris app
            run_application(run_tetris);
        } else if (keq(msg.data.key, (struct key_coord){0, 0})) {
            open_page(state, UI_PAGE_IDLE);
            return;
        }
    }
    lcd_goto_xpix_y(0, 0);
    lcd_clear_buffer();
    lcd_puts("wake + <key> to start\n");
    lcd_puts("B: breakout G: GOL\n");
    lcd_puts("L: lander   I: mines\n");
    lcd_puts("S: snake    T: tetris\n");
    lcd_puts("M: mandelbrot\n");
    lcd_goto_xpix_y(0, 6);
    lcd_puts("X: exit");
    lcd_display();
}

void show_idle_page(struct ui_message msg, struct ui_state *state) {
    show_animation(&state->page_state.anim, &anim_idle, true);
}

struct ui_page_cfg {
    void (*show)(struct ui_message, struct ui_state *state);
    enum ui_message_type
        trigger_msg;               // type of message that triggers this page
    struct key_coord trigger_key;  // press wake + trigger_key to show this page
    bool allow_navigation;  // if true, respond to triggers of other pages
};

// Used for pages that don't require a key press to show
#define NO_KEY {42, 42}

struct ui_page_cfg ui_page_cfgs[__UI_N_PAGES] = {
    {NULL, UI_MESSAGE_TYPE_NOMSG, NO_KEY, true},
    {show_shutdown_page, UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED, {1, 6}, false},
    {show_debug_page, UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED, {1, 10}, true},
    {show_confirm_passkey_page, UI_MESSAGE_TYPE_CONFIRM_PASSKEY, NO_KEY, false},

    {show_display_passkey_page, UI_MESSAGE_TYPE_DISPLAY_PASSKEY, NO_KEY, true},
    {show_startup_page, UI_MESSAGE_TYPE_STARTUP, NO_KEY, false},
    {show_swap_ctrl_cmd_page,
     UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED,
     {0, 4},
     true},
    {show_help_page, UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED, {0, 7}, true},
    {show_apps_page, UI_MESSAGE_TYPE_WAKE_AND_KEY_PRESSED, {1, 2}, false},
    {show_idle_page, UI_MESSAGE_TYPE_WAKE_PRESSED, NO_KEY, true},
};

void switch_page(struct ui_state *state, struct ui_message *msg) {
    for (int i = 0; i < __UI_N_PAGES; i++) {
        struct ui_page_cfg cfg = ui_page_cfgs[i];
        if (cfg.trigger_msg == msg->type &&
            (keq(cfg.trigger_key, (struct key_coord){42, 42}) ||
             keq(cfg.trigger_key, msg->data.key))) {
            if (state->current_page != i) {
                open_page(state, (enum ui_page)i);
            }
            break;
        }
    }
}

static struct ui_state current_ui_state;

void switch_off(struct ui_state *state) {
    lcd_clear_buffer();
    lcd_display();
    k_msleep(10);
    disable_display();
    state->current_page = UI_DISABLED;
}

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
        if (state->current_page != UI_DISABLED) {
            // if the display is active update every UI_TIMESTEMP_MS
            ret = k_msgq_get(&ui_messages, &msg, K_MSEC(UI_TIME_STEP_MS));
        } else {
            // else sleep until we get a ui message
            ret = k_msgq_get(&ui_messages, &msg, K_FOREVER);
        }
        if (ret == 0) {
            // got a message, make sure display is on
            if (state->current_page == UI_DISABLED) {
                display_init();
            }
            state->last_msg_time = k_uptime_get();

            if (ui_page_cfgs[state->current_page].allow_navigation) {
                switch_page(state, &msg);
            }
        }
        if ((k_uptime_get() - state->last_msg_time) > UI_TIMEOUT_MS) {
            switch_off(state);
            continue;
        }
        if (state->current_page != UI_DISABLED) {
            ui_page_cfgs[state->current_page].show(msg, state);
        }
    }
}

void suspend_ui() { k_thread_suspend(ui_thread_id); }

void resume_ui() { k_thread_resume(ui_thread_id); }

void init_ui(void) {
    k_msgq_init(&ui_messages, ui_message_buffer, sizeof(struct ui_message),
                UI_MSG_QUEUE_SIZE);
    disable_display();
    ui_thread_id =
        k_thread_create(&ui_thread_data, ui_thread_stack,
                        K_THREAD_STACK_SIZEOF(ui_thread_stack), ui_thread, NULL,
                        NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}
