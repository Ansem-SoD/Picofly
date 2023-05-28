#include "pico/stdlib.h"
#include "config.h"
#include "fuses.h"
#include <stdio.h>
#include <string.h>

typedef struct usk_cfg {
    uint8_t cid[16];
    uint8_t sign;
    uint8_t version_hi;
    uint8_t version_lo;
    uint8_t lock;
    uint16_t pcrc;
} usk_cfg;

#define VER_SIGN sizeof(usk_cfg)

uint16_t payload_crc();

#define typed_cfg ((const usk_cfg *)flash_cfg)

bool is_inited() {
    return typed_cfg->sign == VER_SIGN 
    && typed_cfg->version_hi == VER_HI 
    && typed_cfg->version_lo == VER_LO
    && payload_crc() == typed_cfg->pcrc;
}

bool is_configured(uint8_t * cid) {
    bool cid_ok = memcmp(typed_cfg->cid, cid, 16) == 0;
    bool init_ok = is_inited();
    return cid_ok && init_ok;
}

bool check_blank_config(int start, int size) {
    for(int i = start; i < start + size; i+= 4) {
        uint32_t val = *(volatile uint32_t*)(flash_cfg + i);
        if (val != 0xFFFFFFFF)
            return false;
    }
    return true;
}

void erase_config() {
    for (int i = 0; i < CONFIG_SIZE; i += 0x1000) {
        if (!check_blank_config(i, 0x1000)) {
            flash_range_erase(CONFIG_START + i, 0x1000);
        }
    }
}

bool is_locked() {
    return typed_cfg->lock != 0xFF;
}

void lock_config() {
    if (is_locked())
        return;
    uint8_t buf[256];
    usk_cfg * cfg = (usk_cfg *)buf;
    memcpy(buf, flash_cfg, 256);
    cfg->lock = 0;
    flash_range_program(CONFIG_START, buf, 256);
}

extern int boot_slot;

void init_config(uint8_t * cid) {
    erase_config();
    uint8_t buf[256];
    memset(buf, 0xFF, 256);
    usk_cfg * cfg = (usk_cfg *)buf;
    memcpy(cfg->cid, cid, 16);
    cfg->sign = VER_SIGN;
    cfg->version_hi = VER_HI;
    cfg->version_lo = VER_LO;
    cfg->pcrc = payload_crc();
    flash_range_program(CONFIG_START, buf, 256);
}
void add_boot_record(int offset) {
    if (is_locked()) {
        return;
    }
    uint32_t buf[64];
    memcpy(buf, flash_cfg + CONFIG_IDX(offset) * 256, 256);
    for (int i = 0; i < 64; i++) {
        if (buf[i] != 0) {
            int tz = __builtin_ctz(buf[i]);
            buf[i] &= ~(1 << tz);
            flash_range_program(CONFIG_START + CONFIG_IDX(offset) * 256, (uint8_t*)buf, 256);
            return;
        }
    }
    lock_config();
}

int get_weigth(int offset) {
    int weight = 0;
    uint32_t * buf = (uint32_t*) (flash_cfg + CONFIG_IDX(offset) * 256);
    for (int i = 0; i < 64; i++) {
        if(buf[i] == 0)
            weight += 32;
        else {
            weight += __builtin_ctz(buf[i]);
            break;
        }
    }
    return weight;
}

int find_best_record(int * max_weight) {
    static int last_weight = -1;
    static int last_offset = 0;
    int best_offset = -1;
    int best_weight = 0;
    for (int i = OFFSET_MIN; i < OFFSET_MAX; i += OFFSET_DIV) {
        int cur_weight = get_weigth(i);
        if (cur_weight > best_weight) {
            if (*max_weight == -1 || cur_weight < *max_weight
            || (cur_weight == *max_weight && *max_weight == last_weight &&
                last_offset < i)) {
                best_offset = i;
                best_weight = cur_weight;
            }
        }
    }
    if (best_offset != -1) {
        *max_weight = best_weight;
    }
    last_weight = best_weight;
    last_offset = best_offset;
    return best_offset;
}