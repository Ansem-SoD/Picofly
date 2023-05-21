#include "stdbool.h"

bool detect_by_pull_up(int frc_pin, int det_pin);

void detect_board();
int gli_pin();
int pwr_pin();
int led_pin();
bool is_pico();
int scl_pin();
int sda_pin();