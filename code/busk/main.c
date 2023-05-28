#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "pins.h"

#include "firmware.h"

void pins_setup(void)
{
	// avoid mosfet enable no matter what
	gpio_pull_down(PIN_GLI_WS);
	gpio_pull_down(PIN_GLI_XIAO);
	gpio_pull_down(PIN_GLI_PICO);
	gpio_pull_down(PIN_GLI_ITSY);
	*(uint32_t*)(0x4001c000 + 4 + PIN_RST*4 + 0x2000) = 0x81;
}

// overclock setup
void oc_setup(void)
{
	vreg_set_voltage(VREG_VOLTAGE_1_30);
	set_sys_clock_khz(200000, true);
}

void zzz() {
    *(uint32_t*)(0x4000803C + 0x3000) = 1;  // go to 12 MHz
    uint32_t * vreg = (uint32_t*)0x40064000;
    vreg[1] &= ~1;  // disable brownout
    *(uint32_t*)0x40060000 = 0x00d1e000; // disable rosc
    *(uint32_t*)0x40004018 = 0xFF;  // disable SRAMs
	vreg[0] = 1;    // lowest possible power
    *(uint32_t*)0x40024000 = 0x00d1e000; // disable xosc
    while(1);
}

void halt() {
	// disable all pins except glitch
	for(int pin = 0; pin <= 29; pin += 1) {
        uint32_t pad_reg = 0x4001c000 + 4 + pin*4;
		// glitch pin pulldown, just in case
        if (pin == PIN_GLI_PICO || pin == PIN_GLI_XIAO || pin == PIN_GLI_WS || pin == PIN_GLI_ITSY)
            *(uint32_t*)pad_reg = 0x85; //pulldown
        else
            *(uint32_t*)pad_reg = 0x81;
    }
    zzz();
}

#define fw_slot_0 ((struct fw_header *) (XIP_BASE + 0x10000))
#define fw_slot_1 ((struct fw_header *) (XIP_BASE + 0x48000))
#define RAM ((uint8_t *) 0x20000000)

#define FUSE_OFF 0xF000

#define fuses ((const uint8_t *) (XIP_BASE + FUSE_OFF))

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

void perform_reboot()
{
	watchdog_enable(0, false);
	while(1);
}

void startup_from_slot(int slot)
{
	struct fw_header * fw = slot ? fw_slot_1 : fw_slot_0;
	watchdog_enable(100, false);
	// set the slot where we have started from
	watchdog_hw->scratch[1] = slot;
	// check the size
	if (fw->size > 0x38000 || fw->size == 0) {
		perform_reboot();
	}
	// copy data into the RAM
	memcpy(RAM, fw->data, fw->size);
	uint32_t real_crc = crc32(RAM, fw->size);
	if (real_crc != fw->crc)
	{
		perform_reboot();
	}
	// jump to the unpacked firmware
	((nopar*)0x20000001)();
}

int main(void)
{
	pins_setup();
	oc_setup();
	int boot_slot = (count_fuses() & 1);
	if (!watchdog_enable_caused_reboot() || !watchdog_caused_reboot()) // first boot attempt
	{
		watchdog_hw->scratch[0] = 0;
		startup_from_slot(boot_slot & 1);
	}
	else if (watchdog_caused_reboot()) // next boot attempt
	{
		int boot_try = watchdog_hw->scratch[0];
		if (boot_try == 3) // 1 -> update or failed boot, 2->rollback after update, 3- total fail
		{
			halt();
		}
		watchdog_hw->scratch[0] = boot_try + 1;
		startup_from_slot((boot_slot ^ boot_try ^ 1) & 1);
	}
	return 0;
}