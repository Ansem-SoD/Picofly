#include "fuses.h"
#include "stdint.h"
#include "string.h"
#include "hardware/watchdog.h"

// firmware info from the bootloader & flash
int boot_slot = 0;
int boot_try = 0;

// firmware update fuse count calculator
int count_fuses()
{
    int weight = 0;
    uint32_t * buf = (uint32_t*)fuses;
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

// firmware update fuse burner
void burn_fuse()
{
    uint32_t buf[64];
    memcpy(buf, fuses, 256);
    for (int i = 0; i < 64; i++) {
        if (buf[i] != 0) {
            int tz = __builtin_ctz(buf[i]);
            buf[i] &= ~(1 << tz);
            flash_range_program(FUSE_OFF, (uint8_t*)buf, 256);
            return;
        }
    }
	flash_range_erase(FUSE_OFF, 0x1000);
	burn_fuse(); // burn one more because both 0 and 256*8 are even
}

void init_fuses()
{
    boot_slot = watchdog_hw->scratch[1];
    boot_try = watchdog_hw->scratch[0];
}