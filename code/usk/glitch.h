#include "config.h"
#define GLITCH_RESULT_SUCCESS 0 // glitch worked and loaded
#define GLITCH_RESULT_MISSING 1 // trigger is missing
#define GLITCH_RESULT_FAILURE 2 // glitch did not work
#define GLITCH_RESULT_TIMEOUT 3 // glitch worked, but timed out
#define G_SNIFF_SM 0
#define G_TRIG_SM  1
#define G_DAT0_SM  2
int do_glitch(int delay, int width, int total_ms, int after_ms);
void init_glitch_pio();
void deinit_glitch_pio();
bool glitch_try_offset(int offset, int * width, int edge_limit);
void prepare_random_array();
extern int offsets_array[OFFSET_CNT];