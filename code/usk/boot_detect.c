#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pins.h"
#include "pico/stdlib.h"
#include "glitch.h"
#include "misc.h"
#include "fuses.h"

bool mariko = true;
bool board_detected = false;

bool wait_for_boot(int timeout_ms) {
    absolute_time_t tio_full = make_timeout_time_ms(timeout_ms);
    absolute_time_t tio_cmd1 = tio_full;
    init_glitch_pio();
    reset_cpu();
    uint32_t word=0, last_word=0;
    bool was_read_zero = false;
    bool was_cmd1 = false;
    int reset_attempts = 0;

    while(!time_reached(tio_full)) {
        if (time_reached(tio_cmd1))
        {
            if (reset_attempts > 4)
            {
                halt_with_error(0, 3);
            }
            reset_attempts++;
            reset_cpu();
            tio_cmd1 = tio_full;
        }
        if(!pio_sm_is_rx_fifo_empty(pio1, 0))
        {
            word = pio_sm_get(pio1, 0);
            if (last_word == 0x41000000 && word == 0x00F9) // cmd1 request
            {
                tio_cmd1 = make_timeout_time_ms(20);
                was_cmd1 = true;
            }
            else if (last_word == 0x00F9 && (word >> 24) == 0x3F) // cmd1 responce
            {
                tio_cmd1 = tio_full;
            }
            else if (last_word == 0x51000000 && word == 0x0055) //read block 0
            {
                tio_full = make_timeout_time_ms(100);
                was_read_zero = true;
            } else if (was_read_zero && last_word == 0x4D000200 && word == 0x00B1) // read status - erista only
            {
                mariko = false;
            } else if (last_word == 0x51000000 && word == 0x0147) // read block 1, can finish now
            {
                deinit_glitch_pio();
                return true;
            }
            last_word = word;
        }
    }
    if (was_read_zero) {
        halt_with_error(1, 3);
    }
    else if (was_cmd1) {
        halt_with_error(2, 3);
    } else {
        halt_with_error(3, 3);
    }
    return false;
}