#define PIX_blu 0x00003F
#define PIX_yel 0x151500
#define PIX_whi 0x111111

#define PIX_b 0x00000F

void put_pixel(uint32_t pixel_grb);

void halt_with_error(uint32_t err, uint32_t bits);

void gpio_disable_input_output(int pin);

void gpio_enable_input_output(int pin);

void finish_pins_except_leds();

void reset_cpu();