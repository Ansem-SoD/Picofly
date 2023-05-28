

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "emmc.pio.h"
#include "pins.h"
#include "glitch.h"
#include <stdio.h>
#include "misc.h"
#include "board_detect.h"
#include "hardware/structs/rosc.h"

extern uint32_t gsniff_pio_offset;
extern uint32_t dsniff_pio_offset;
extern uint32_t gtrig_pio_offset;

void init_gsniff_pio() {
    pio_sm_config c = glitch_sniff_cmd_program_get_default_config(gsniff_pio_offset);
    sm_config_set_in_pins(&c, PIN_CMD);
    sm_config_set_jmp_pin(&c, PIN_CMD);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio1, G_SNIFF_SM, gsniff_pio_offset, &c);
    pio_sm_set_enabled(pio1, G_SNIFF_SM, true);
}

void init_dat0_pio() {
    pio_sm_config c = glitch_dat_waiter_program_get_default_config(dsniff_pio_offset);
    sm_config_set_in_pins(&c, PIN_DAT);
    sm_config_set_jmp_pin(&c, PIN_DAT);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_out_shift(&c, false, false, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio1, G_DAT0_SM, dsniff_pio_offset, &c);
    pio_sm_set_enabled(pio1, G_DAT0_SM, true);
}

extern bool mariko;

void init_trigger_pio() {
    uint offset = gtrig_pio_offset;
    pio_sm_config c = glitch_trigger_program_get_default_config(offset);
    sm_config_set_sideset_pins (&c, gli_pin());
    sm_config_set_out_shift(&c, false, true, 32);
    
    pio_sm_set_consecutive_pindirs(pio1, G_TRIG_SM, gli_pin(), 1, true);
    pio_sm_init(pio1, G_TRIG_SM, offset, &c);
    pio_sm_set_enabled(pio1, G_TRIG_SM, true);
    pio_gpio_init(pio1, gli_pin());
    gpio_set_slew_rate(gli_pin(), GPIO_SLEW_RATE_FAST);
}

void init_glitch_pio() {
    for (int i = PIN_CLK; i <= PIN_DAT; i++)
    {
        gpio_init(i);
        gpio_enable_input_output(i);
        gpio_pull_up(i);
    }
    gpio_init(gli_pin());
    init_gsniff_pio();
    init_dat0_pio();
    init_trigger_pio();
}

void deinit_glitch_pio() {
    pio_set_sm_mask_enabled(pio1, 0x3, false);
    for (int i = PIN_CLK; i <= PIN_DAT; i++)
    {
            gpio_deinit(i);
            gpio_disable_pulls(i);
            gpio_disable_input_output(i);
    }
    gpio_deinit(gli_pin());
}

int do_glitch(int delay, int width, int total_ms, int after_ms) {
    // at this point CPU must be reset
    init_glitch_pio();
    pio_sm_put_blocking(pio1, G_TRIG_SM, delay);
    pio_sm_put_blocking(pio1, G_TRIG_SM, width);

    bool glitch_started = false;

    uint32_t last_word = 0;
    
    uint32_t data_count = 0;

    int result = GLITCH_RESULT_MISSING;
    bool detected = false;

    absolute_time_t detect_time = make_timeout_time_ms(20);
    absolute_time_t full_time = make_timeout_time_ms(total_ms);
    absolute_time_t tio_time = detect_time;

    while (!time_reached(tio_time)) {
        if (!pio_sm_is_rx_fifo_empty(pio1, G_DAT0_SM)) {
            uint32_t temp = pio_sm_get(pio1, G_DAT0_SM);
        }
        if (!pio_sm_is_rx_fifo_empty(pio1, G_SNIFF_SM)) {
           uint32_t cur_word = pio_sm_get(pio1, G_SNIFF_SM);
           if (!detected && (cur_word >> 24) == 0x3F) {
               tio_time = full_time;
               detected = true;
           }
           if (glitch_started)
               data_count += 1;
           if (last_word == 0x51000000 && cur_word == 0x1351) {
               if (!glitch_started)
                   tio_time = make_timeout_time_ms(after_ms);
               glitch_started = true;
           }
           if (last_word == 0x51000000 && cur_word == 0x142F) {
               result = GLITCH_RESULT_FAILURE;
               break;
           }
           if (last_word == 0x40aa5458 && cur_word == 0xba3b) {
               result = GLITCH_RESULT_SUCCESS;
               break;
           }
           if (data_count == 10) {
               tio_time = full_time;
           }
           last_word = cur_word;
        }
    }
    deinit_glitch_pio();
    if (result == GLITCH_RESULT_MISSING && glitch_started) {
        result = GLITCH_RESULT_TIMEOUT;
    }
    return result;
}

int tries = 0;

// Blue pulsing implementation. 
void inc_tries()
{
    tries += 1;
    if(tries & 1)
        put_pixel(PIX_b);
    else
        put_pixel(PIX_blu);
}

// random() for glitch offset array generation
static uint32_t get_random_word(int cycles) {
    uint32_t byte;
    assert(rosc_hw->status & ROSC_STATUS_ENABLED_BITS);
    for(int i=0; i < cycles; i++) {
        busy_wait_at_least_cycles(100);
        byte = ((byte << 1) | rosc_hw->randombit);
    }
    return byte;
}

int offsets_array[OFFSET_CNT];
void prepare_random_array()
{
    for(int i = 0; i < OFFSET_CNT; i++)
        offsets_array[i] = OFFSET_MIN + OFFSET_DIV*i;
    for (int i = OFFSET_CNT - 1; i > 0; i--) {
            size_t j = get_random_word(32) % (i+1);
            int t = offsets_array[j];
            offsets_array[j] = offsets_array[i];
            offsets_array[i] = t;
    }
}


bool glitch_try_offset(int offset, int * width, int edge_limit) {
    int last_res = GLITCH_RESULT_SUCCESS;
    int edges = 0;
    int step = tries == 0 ? 32 : 8; // first ever edge search should be faster
    int missing_count = 0;
    inc_tries();
    while (1) {
        reset_cpu();
        int gres = do_glitch(offset + get_random_word(3), *width, 300, 6);
        if (gres == GLITCH_RESULT_MISSING)
            missing_count +=1; // MMC has failed to initialize
        else
            missing_count = 0;
        if (missing_count >= 5)
            halt_with_error(4, 3); // something wrong with eMMC, cannot init properly
        if (*width == 1) {
            halt_with_error(5, 3);
        }
        if (*width >= 1000) {  // no reaction to the glitch, hardware failure or bad mosfet wire
            halt_with_error(6, 3);
        }
        if ((last_res == GLITCH_RESULT_TIMEOUT && gres == GLITCH_RESULT_FAILURE)
          || (gres == GLITCH_RESULT_TIMEOUT && last_res == GLITCH_RESULT_FAILURE)) {
            if (step != 1)
                step /= 2; // we have found the edge, now increase search presicion
            else
                edges += 1; // max precision, start walking around the edge
        }
        last_res = gres;
        if (gres == GLITCH_RESULT_FAILURE) // not enough glitch, need more width
            *width += step;
        if (gres == GLITCH_RESULT_TIMEOUT) { // too much glitch, need less width
            *width -= step;
            if (*width <= 1)
                *width = 1;
        }
        if (gres == GLITCH_RESULT_SUCCESS)
            return true;
        if (edges > edge_limit) // this offset did not work
            return false;
    }
}
