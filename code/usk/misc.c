#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "pins.h"
#include <string.h>
#include "hardware/vreg.h"
#include "ws2812.pio.h"
#include "board_detect.h"
#include "misc.h"
#include "board_detect.h"

extern int ws_pio_offset;

#define BLINK_TIME 700
#define SHORT_TIME ( BLINK_TIME * 2 / 10 )
#define SHORT_PAUSE_TIME ((BLINK_TIME - SHORT_TIME) / 2)
#define LONG_TIME ( BLINK_TIME * 8 / 10 )
#define LONG_PAUSE_TIME ((BLINK_TIME - LONG_TIME) / 2)
#define PAUSE_BETWEEN 2000
#define PAUSE_BEFORE 750
#define CODE_REPEATS 3

#define GPIO_OD PADS_BANK0_GPIO0_OD_BITS
#define GPIO_IE PADS_BANK0_GPIO0_IE_BITS
#define GPIO_OD_PD (GPIO_OD | PADS_BANK0_GPIO0_PDE_BITS)

#define PTR_SAFE_RAM4 (void*)0x20040200

typedef void nopar();

void __not_in_flash_func(zzz)() {
    static bool not_first = 0;
    if(!not_first)
    {
        not_first = 1;
        memcpy(PTR_SAFE_RAM4, (void*)((uint32_t)zzz - 1), 0x200);
        ((nopar*)(PTR_SAFE_RAM4 + 1))();
    }
    *(uint32_t*)(0x4000803C + 0x3000) = 1;  // go to 12 MHz
    uint32_t * vreg = (uint32_t*)0x40064000;
    vreg[1] &= ~1;  // disable brownout
    *(uint32_t*)0x40060000 = 0x00d1e000; // disable rosc
    *(uint32_t*)0x40004018 = 0xFF ^ (1 << 4);  // disable SRAMs except ours
    vreg[0] = 1;    // lowest possible power
    *(uint32_t*)0x40024000 = 0x00d1e000; // disable xosc
    while(1);
}

void finish_pins_except_leds() {
    for(int pin = 0; pin <= 29; pin += 1) {
        if (pin == led_pin() || pin == pwr_pin())
            continue;
        if (pin == PIN_GLI_PICO || pin == PIN_GLI_XIAO || pin == PIN_GLI_WS || pin == PIN_GLI_ITSY)
        {
            gpio_pull_down(pin);
        }
        else
        {
            gpio_disable_pulls(pin);
        }
        gpio_disable_input_output(pin);
    }
}

void finish_pins_leds() {
    gpio_disable_input_output(led_pin());
    gpio_disable_input_output(pwr_pin());
}

void halt_with_error(uint32_t err, uint32_t bits)
{
    finish_pins_except_leds();
    pio_set_sm_mask_enabled(pio0, 0xF, false);
    pio_set_sm_mask_enabled(pio1, 0xF, false);
    set_sys_clock_khz(48000, true);
    vreg_set_voltage(VREG_VOLTAGE_0_95);
    if (bits != 1)
    {
        put_pixel(0);
        sleep_ms(PAUSE_BEFORE);
    }
    for(int j = 0; j < CODE_REPEATS; j++)
    {
        for(int i = 0; i < bits; i++)
        {
            bool is_long = err & (1 << (bits - i - 1));
            sleep_ms(is_long ? LONG_PAUSE_TIME : SHORT_PAUSE_TIME);
            bool success = bits == 1 && is_long == 0;
            if (success)
                put_pixel(PIX_whi);
            else
                put_pixel(PIX_yel);
            sleep_ms(is_long ? LONG_TIME : success ? SHORT_TIME * 2 : SHORT_TIME);
            put_pixel(0);
            if (i != bits - 1 || j != CODE_REPEATS - 1)
                sleep_ms(is_long ? LONG_PAUSE_TIME : SHORT_PAUSE_TIME);
            if (i == bits - 1 && j != CODE_REPEATS - 1)
                sleep_ms(PAUSE_BETWEEN);
        }
        // first write case, do not repeat this kind of error code
        if (bits == 1)
            break;
    }
    finish_pins_leds();
    zzz();
}

void put_pixel(uint32_t pixel_grb)
{
    static bool led_enabled = false;
    if (is_pico())
    {
        gpio_init(led_pin());
        if (pixel_grb) {
            gpio_set_dir(led_pin(), true);
            gpio_put(led_pin(), 1);
        }
        return;
    }
    ws2812_program_init(pio0, 3, ws_pio_offset, led_pin(), 800000, true);
    if (!led_enabled && pwr_pin() != 31)
    {
        led_enabled = true;
        gpio_init(pwr_pin());
        gpio_set_drive_strength(pwr_pin(), GPIO_DRIVE_STRENGTH_12MA);
        gpio_set_dir(pwr_pin(), true);
        gpio_put(pwr_pin(), 1);
        sleep_us(200);
    }
    pio_sm_put_blocking(pio0, 3, pixel_grb << 8u);
    sleep_us(50);
    pio_sm_set_enabled(pio0, 3, false);
    gpio_init(led_pin());
}

void gpio_disable_input_output(int pin)
{
    uint32_t pad_reg = 0x4001c000 + 4 + pin*4;
    *(uint32_t*)(pad_reg + 0x2000) = GPIO_OD;
    *(uint32_t*)(pad_reg + 0x3000) = GPIO_IE;
}

void gpio_enable_input_output(int pin)
{
    uint32_t pad_reg = 0x4001c000 + 4 + pin*4;
    *(uint32_t*)(pad_reg + 0x3000) = GPIO_OD;
    *(uint32_t*)(pad_reg + 0x2000) = GPIO_IE;
}

void reset_cpu() {
    gpio_enable_input_output(PIN_RST);
    gpio_pull_up(PIN_RST);
    sleep_us(1000);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, true);
    sleep_us(2000);
    gpio_deinit(PIN_RST);
    gpio_disable_pulls(PIN_RST);
    gpio_disable_input_output(PIN_RST);
}
