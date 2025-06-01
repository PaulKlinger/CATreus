#include "nvs.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <pm_config.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "config.h"
#include "bluetooth.h"

#define NVS_PARTITION		custom_nvs_storage
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

static struct nvs_fs fs;

#define N_BOOT 1
#define CTRL_CMD 2

struct ctrl_cmd_config {
    bool swap_ctrl_cmd;
    char addr[BT_ADDR_LE_STR_LEN];
};

struct ctrl_cmd_configs {
    struct ctrl_cmd_config configs[CONFIG_BT_MAX_PAIRED];
    uint8_t n_configs;
};

int nvs_init() {
    fs.flash_device = NVS_PARTITION_DEVICE;
    fs.offset = NVS_PARTITION_OFFSET;
    struct flash_pages_info info;
    int rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
    fs.sector_size = info.size;
	fs.sector_count = 3U;

    
	rc = nvs_mount(&fs);
	if (rc) {
		printk("Flash Init failed, rc=%d\n", rc);
		return 1;
	}
    printk("NVS mounted successfully.\n");
    return 0;
}

void nvs_store_n_boot(uint16_t value) {
    nvs_write(&fs, N_BOOT, &value, sizeof(value));
}   

uint16_t nvs_read_n_boot() {
    uint16_t value;
    int ret = nvs_read(&fs, N_BOOT, &value, sizeof(value));
    return (ret < 0) ? 0 : value;
}

struct ctrl_cmd_configs nvs_read_ctrl_cmd_configs() {
    struct ctrl_cmd_configs cfgs = {0};
    int ret = nvs_read(&fs, CTRL_CMD, &cfgs, sizeof(cfgs));
    
    return (ret < 0) ? (struct ctrl_cmd_configs){0} : cfgs;
}


void nvs_store_ctrl_cmd(bool swap_ctrl_cmd) {
    struct addr addr = get_current_addr();
    struct ctrl_cmd_configs cfgs = nvs_read_ctrl_cmd_configs();
    bool found = false;
    for (int i = 0; i < cfgs.n_configs; i++) {
        if (strcmp(cfgs.configs[i].addr, addr.addr) == 0) {
            found = true;
            cfgs.configs[i].swap_ctrl_cmd = swap_ctrl_cmd;
        }
    }
    if (!found) {
        struct ctrl_cmd_config new_config = {
            .swap_ctrl_cmd = swap_ctrl_cmd,
        };
        strcpy(new_config.addr, addr.addr);
        if (cfgs.n_configs >= CONFIG_BT_MAX_PAIRED) {
            // storage full, get rid of oldest config
            for (int i = 0; i < CONFIG_BT_MAX_PAIRED - 1; i++) {
                cfgs.configs[i] = cfgs.configs[i + 1];
            }
            cfgs.n_configs--;
        }
        cfgs.configs[cfgs.n_configs] = new_config;
        cfgs.n_configs++;
    }
    
    nvs_write(&fs, CTRL_CMD, &cfgs, sizeof(cfgs));
}

bool nvs_get_ctrl_cmd_config() {
    struct ctrl_cmd_configs cfgs = nvs_read_ctrl_cmd_configs();
    struct addr addr = get_current_addr();
    for (int i = 0; i < cfgs.n_configs; i++) {
        if (strcmp(cfgs.configs[i].addr, addr.addr) == 0) {
            return cfgs.configs[i].swap_ctrl_cmd;
        }
    }
    return false;
}