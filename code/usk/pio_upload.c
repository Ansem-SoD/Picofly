#include "glitch.h"
#include "hardware/pio.h"
#include "emmc.pio.h"
#include "ws2812.pio.h"

uint32_t ws_pio_offset = 0;
uint32_t clk_pio_offset = 0;
uint32_t sdin_pio_offset = 0;
uint32_t sdout_pio_offset = 0;
uint32_t gsniff_pio_offset = 0;
uint32_t dsniff_pio_offset = 0;
uint32_t gtrig_pio_offset = 0;

// PIO is programmed once, originally this was made to protect the PIO code 
void upload_pio()
{
    // LED programming PIO
    ws_pio_offset = pio_add_program(pio0, &ws2812_program);

    // proper shift settings for the value setup
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_shift(&c, false, true, 32);

    // MMC CMD sniffer
    gsniff_pio_offset = pio_add_program(pio1, &glitch_sniff_cmd_program);
    pio1->sm[G_SNIFF_SM].shiftctrl = c.shiftctrl;
    // set OSR to 48 - 1 - 1 (number of bits in each command)
    uint val = 46;
    for (int i = 1; i >= 0; i--) {
        pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_set(pio_y, (val >> i*4) & 15));
        pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_in(pio_y, 4));
    }
    pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_mov(pio_osr, pio_isr));
    pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_mov(pio_isr, pio_null));
    // set y to trigger value (last byte of block ID + CRC7)
    val = 0x1351;
    for (int i = 3; i >= 0; i--) {
        pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_set(pio_y, (val >> i*4) & 15));
        pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_in(pio_y, 4));
    }
    pio_sm_exec(pio1, G_SNIFF_SM, pio_encode_mov(pio_y, pio_isr));

    // MMC DAT sniffer
    dsniff_pio_offset = pio_add_program(pio1, &glitch_dat_waiter_program);
    pio1->sm[G_DAT0_SM].shiftctrl = c.shiftctrl;
    // set y to amount of data ticks to skip
    val = 528 * 300 / 25;
    for (int i = 5; i >= 0; i--) {
        pio_sm_exec(pio1, G_DAT0_SM, pio_encode_set(pio_y, (val >> i*4) & 15));
        pio_sm_exec(pio1, G_DAT0_SM, pio_encode_in(pio_y, 4));
    }
    pio_sm_exec(pio1, G_DAT0_SM, pio_encode_mov(pio_y, pio_isr));

    // glitch trigger PIO
    gtrig_pio_offset = pio_add_program(pio1, &glitch_trigger_program);

    // MMC payload read/write PIOs
    clk_pio_offset = pio_add_program(pio0, &sd_clk_program);
    sdout_pio_offset = pio_add_program(pio0, &out_cmd_or_dat_program);
    sdin_pio_offset = pio_add_program(pio0, &in_cmd_or_dat_program);
}
