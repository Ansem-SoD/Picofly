#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/xosc.h"
#include "hardware/watchdog.h"
#include "boot_detect.h"
#include "board_detect.h"
#include "config.h"

#include "misc.h"
#include "glitch.h"
#include "fuses.h"
#include "pio_upload.h"
#include "pins.h"

bool write_payload();

// overclock to 300 MHz
void init_system() {
    vreg_set_voltage(VREG_VOLTAGE_1_30);
	set_sys_clock_khz(300000, true);
}

// filled within "fast check" on eMMC init
extern uint8_t cid_buf[17];

void rewrite_payload()
{
    put_pixel(PIX_whi);
    write_payload();
    put_pixel(PIX_blu);
    // used to automatically rewrite payload when eMMC/console changes
    init_config(cid_buf + 1);
}

bool safe_test_voltage(int pin, float target, float range)
{
    gpio_enable_input_output(pin);
    adc_gpio_init(pin);
    adc_select_input(pin - 26);
    uint16_t result = adc_read();
    gpio_disable_input_output(pin);
    float voltage = result * 3.3f / (1 << 12);
    return voltage >= (target - range) && voltage <= (target + range);
}

// test all ADC pins
void self_test()
{
    absolute_time_t tio_time = make_timeout_time_ms(2500);
    adc_init();
    bool rst_ok = false, cmd_ok = false, d0_ok = false, clk_ok = false;
    while (!time_reached(tio_time)) {
        if (!rst_ok)
            rst_ok |= safe_test_voltage(PIN_RST, 1.8f, 0.2f);
        if (!cmd_ok)
            cmd_ok |= safe_test_voltage(PIN_CMD, 1.8f, 0.2f);
        if (!d0_ok)
            d0_ok |= safe_test_voltage(PIN_DAT, 1.8f, 0.2f);
        if (rst_ok && cmd_ok && d0_ok)
            break;
    }
    if(!rst_ok)
    {
        halt_with_error(0, 2);
    }
    if(!cmd_ok)
    {
        halt_with_error(1, 2);
    }
    if(!d0_ok)
    {
        halt_with_error(2, 2);
    }
}

extern bool was_self_reset;

int main()
{
    // stop watchdog
    *(uint32_t*)(0x40058000 + 0x3000) = (1 << 30);
    // init reset, mosfet and LED
    detect_board();
    // clocks & voltage
	init_system();
    // fuses counter
    init_fuses();
    // LED & glitch & emmc PIO
    upload_pio();
    // check if this is the very first start
    if (watchdog_caused_reboot() && boot_try == 0)
	{
		halt_with_error(1, 1);
	}
    // is chip reset required
    bool force_button = detect_by_pull_up(1, 0);
    // start LED
    put_pixel(PIX_blu);
    // test pins
    self_test();
    // wait till the CPU has proper power & started reading the eMMC
    wait_for_boot(2500);
    // ensure the BCT has not been overwritten by system update
    bool force_check = fast_check();
    was_self_reset = force_button || !is_configured(cid_buf + 1);
    // perform payload rewrite if required
    if (!force_check || was_self_reset) {
        rewrite_payload();
    }
    // setup the glitch trigger for Mariko
    if (mariko) {
        pio1->instr_mem[gtrig_pio_offset + 4] = pio_encode_nop();
        pio1->instr_mem[gtrig_pio_offset + 5] = pio_encode_nop();
    }
    // start from some default width
    int width = 150;
    bool glitched = false;
    int offset = 0;
    for (int full_try = 0; full_try < 2; full_try++) {
        // try saved records
        for (int y = 0; (y < 2) && !glitched; y++) {
            int max_weight = -1;
            while (1) {
                offset = find_best_record(&max_weight);
                if (offset == -1)
                    break;
                // try glitch
                glitched = glitch_try_offset(offset, &width, 3);
                if (glitched)
                    break;
            }
        }
        if (!glitched) {
            for(int z = 0; (z < 2) && !glitched; z++) {
                prepare_random_array();
                for(int y = 0; y < OFFSET_CNT; y++)
                {
                    offset = offsets_array[y];
                    glitched = glitch_try_offset(offset, &width, 4);
                    if (glitched)
                        break;
                }
            }
        }
        if (glitched) {
            if ((count_fuses() & 1) != boot_slot)
            {
                // finish update / rollback
                burn_fuse();
            }
            add_boot_record(offset);
            halt_with_error(0, 1);
        }
        if (full_try == 0) {
            rewrite_payload();
        }
    }
    // attempts limit
    halt_with_error(7, 3);
}
