#include "stdint.h"

extern uint32_t ws_pio_offset;
extern uint32_t clk_pio_offset;
extern uint32_t sdin_pio_offset;
extern uint32_t sdout_pio_offset;
extern uint32_t gsniff_pio_offset;
extern uint32_t dsniff_pio_offset;
extern uint32_t gtrig_pio_offset;

void upload_pio();