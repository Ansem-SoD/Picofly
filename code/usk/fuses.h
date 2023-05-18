#include "hardware/flash.h"

// firmware info from the bootloader & flash
extern int boot_slot;
extern int boot_try;

int count_fuses();
void init_fuses();
void burn_fuse();

#define FUSE_OFF 0xF000
#define fuses ((const uint8_t *) (XIP_BASE + FUSE_OFF))