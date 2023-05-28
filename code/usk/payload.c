#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "pins.h"
#include "misc.h"
#include "fuses.h"
#include "payload.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "emmc.pio.h"
#include "mmc_defs.h"
#include "board_detect.h"

#define SM_CLK 0
#define SM_CMDIN 1
#define SM_DATIN 2
#define SM_OUT 3

extern uint32_t clk_pio_offset;
extern uint32_t sdin_pio_offset;
extern uint32_t sdout_pio_offset;

uint16_t crc_itu_t_table[256];

/*const uint16_t crc_itu_t_table[256] = {
       0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
       0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
       0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
       0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
       0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
       0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
       0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
       0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
       0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
       0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
       0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
       0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
       0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
       0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
       0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
       0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
       0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
       0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
       0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
       0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
       0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
       0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
       0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
       0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
       0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
       0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
       0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
       0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
       0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
       0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
       0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
       0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};*/

void crc_prepare_table()
{
    const uint16_t LengthCRC = 16;
    const uint16_t polynomial = 0x1021;
    const uint16_t mask = (1 << (LengthCRC - 1));
    for (int32_t i = 0; i < 256; i++)
    {
        uint16_t crc = i << 8;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & mask)
                crc = (crc << 1) ^ polynomial;
            else
                crc <<= 1;
        }
        crc_itu_t_table[i] = crc;
    }
}

extern bool mariko;

static inline uint16_t crc_itu_t_byte(uint16_t crc, const uint8_t data)
{
	return (crc << 8) ^ crc_itu_t_table[((crc >> 8) ^ data) & 0xff];
}

uint16_t crc_itu_t(uint16_t crc, const uint8_t *buffer, size_t len)
{
    static bool table_prepared = false;
    if (!table_prepared)
    {
        crc_prepare_table();
    }
	while (len--)
		crc = crc_itu_t_byte(crc, *buffer++);
	return crc;
}

extern void zzz();

uint16_t payload_crc()
{
    static int pay_crc = -1;
    if(pay_crc == -1)
        pay_crc = crc_itu_t(0, payload, sizeof(payload));
    return pay_crc;
}

int crc7(uint8_t *buffer, int size)
{
	uint8_t crc = 0;
	for (int i = 0; i < size; i++) {
		uint8_t c = buffer[i];
		for (int j = 0; j < 8; j++) {
			crc <<= 1;
			if ((crc ^ c) & 0x80)
				crc ^= 9;
			c <<= 1;
		}
		crc &= 0x7Fu;
	}
	return crc;
}

void __time_critical_func(cmd_write)(uint8_t cmd, uint32_t argument)
{
    uint8_t data[7];
    // switch to the CMD write
    pio_sm_set_enabled(pio0, SM_OUT, false);
    pio_sm_set_out_pins(pio0, SM_OUT, PIN_CMD, 1);
    pio_sm_set_enabled(pio0, SM_OUT, true);

	data[0] = cmd | 0x40;
	*(uint32_t *) &data[1] = __builtin_bswap32(argument);
	data[5] = (crc7(data, 5) << 1) | 1;
    uint32_t fifo[3];
    fifo[0] = 0xFFCF0000 | (data[0] << 8) | data[1];
    fifo[1] = __builtin_bswap32(*(uint32_t*)(data + 2));
    fifo[2] = 0x80000000;
    for (int i = 0; i < 3; i++) {
        pio_sm_put_blocking(pio0, SM_OUT, ~fifo[i]);
    }
}

uint8_t data_buf[514];

bool __time_critical_func(dat_write)()
{
    // switch to the DAT write
    pio_sm_set_enabled(pio0, SM_OUT, false);
    pio_sm_set_out_pins(pio0, SM_OUT, PIN_DAT, 1);
    pio_sm_set_enabled(pio0, SM_OUT, true);
    uint8_t * buffer = data_buf;
	uint16_t crc = crc_itu_t(0, buffer, 512);
    uint32_t words[130];
    int size_bits = 514 * 8 + 2;
    words[0] = ((size_bits ^ 0xFFFF) << 16) | (buffer[0] << 7) | (buffer[1] >> 1);
    int last_one = buffer[1] & 1;
    for (int i = 1; i < 128; i++) {
        words[i] = (last_one << 31) | (buffer[-2 + i*4] << 23) | (buffer[-1 + i*4] << 15) | (buffer[0 + i*4] << 7) | (buffer[1 + i*4] >> 1);
        last_one = buffer[1 + i*4] & 1;
    }
    words[128] = (last_one << 31) | (buffer[510] << 23) | (buffer[511] << 15) | ((crc >> 8) << 7) | ((crc & 0xFF) >> 1);
    words[129] = ((crc & 1) << 31) | (3 << 29);

    for (int i = 0; i < 130; i++) {
        pio_sm_put_blocking(pio0, SM_OUT, ~words[i]);
    }
    while(!pio_sm_is_tx_fifo_empty(pio0, SM_OUT));
    sleep_us(300);
    int attempts = 200;
    while (--attempts) {
        if (gpio_get(PIN_DAT))
            return true;
        sleep_ms(1);
    }
    return false;
}

void __time_critical_func(cmd_read_request)(int size)
{
    // set initial state
    pio_sm_clear_fifos(pio0, SM_CMDIN);
    pio_sm_exec(pio0, SM_CMDIN, pio_encode_jmp(sdin_pio_offset));
    pio_sm_put_blocking(pio0, SM_CMDIN, size * 8 - 1);
}

void __time_critical_func(dat_read_request)(int size)
{
    // set initial state
    pio_sm_clear_fifos(pio0, SM_DATIN);
    pio_sm_exec(pio0, SM_DATIN, pio_encode_jmp(sdin_pio_offset));
    pio_sm_put_blocking(pio0, SM_DATIN, size * 8 - 1);
}

bool __time_critical_func(dat_read_data)(uint8_t * buf, int size)
{
    absolute_time_t tio = make_timeout_time_us(20000);
    while (!time_reached(tio) && size)
    {
        if (!pio_sm_is_rx_fifo_empty(pio0, SM_DATIN))
        {
            tio = make_timeout_time_us(300);
            uint32_t data = pio_sm_get(pio0, SM_DATIN);
            uint32_t data_corrected = data;
            if (size < 4)
                data_corrected <<= (4 - size)*8;
            for(int i = 3; i >= 0 && size > 0; i--)
            {
                *buf = (data_corrected >> (i*8)) & 0xFF;
                buf++;
                size -= 1;
            }
        }
    }
    return size == 0;
}

bool __time_critical_func(cmd_read_data)(uint8_t * buf, int size)
{
    int last_bit = 0;
    uint8_t * b = buf;
    absolute_time_t tio = make_timeout_time_us(1000);
    while (!time_reached(tio) && size)
    {
        if (!pio_sm_is_rx_fifo_empty(pio0, SM_CMDIN))
        {
            tio = make_timeout_time_us(300);
            uint32_t data = pio_sm_get(pio0, SM_CMDIN);
            uint32_t data_corrected = data;
            if (size < 4)
                data_corrected <<= (4 - size)*8;
            data_corrected = (data_corrected >> 1) | (last_bit << 31);
            last_bit = data & 1;
            for(int i = 3; i >= 0 && size > 0; i--)
            {
                *buf = (data_corrected >> (i*8)) & 0xFF;
                buf++;
                size -= 1;
            }
        }
    }
    return size == 0;
}

bool __time_critical_func(simple_cmd_exec)(int cmd, int arg)
{
    cmd_write(cmd, arg);
    pio_sm_exec_wait_blocking(pio0, SM_CMDIN, pio_encode_irq_wait(false, 1));
    return true;
}

bool __time_critical_func(simple_cmd_exec_with_ret_s)(int cmd, int arg, uint32_t * ret)
{
    uint8_t buffer[6];
    cmd_read_request(6);
    cmd_write(cmd, arg);
    bool result = cmd_read_data(buffer, 6);
    if (result) {
        *ret = __builtin_bswap32(*(uint32_t*)(buffer+1));
    }
    return result;
}

bool __time_critical_func(simple_cmd_exec_with_ret)(int cmd, int arg, uint32_t * ret)
{
    bool res = false;
    for(int i = 0; i < 2; i++)
    {
        res = simple_cmd_exec_with_ret_s(cmd, arg, ret);
        if (res)
            break;
        sleep_us(250);
    }
    return res;
}

uint8_t cid_buf[17];

bool __time_critical_func(cmd_exec_cid)()
{
    cmd_read_request(17);
    cmd_write(MMC_ALL_SEND_CID, 0);
    bool result = cmd_read_data(cid_buf, 17);
    if (result)
    {
        if (crc7(cid_buf + 1, 15) != (cid_buf[16] >> 1))
            return false;
    }
    return result;
}

bool __time_critical_func(cmd_mmc_read)(int block)
{
    dat_read_request(514);
    cmd_write(MMC_READ_SINGLE_BLOCK, block);
    bool result = dat_read_data(data_buf, 514);
    if (result)
    {
        if (crc_itu_t(0, data_buf, 512) != ((data_buf[512] << 8) | data_buf[513]))
            return false;
    }
    return result;
}

bool __time_critical_func(cmd_mmc_write)(int block)
{
    uint32_t ret;
    if(!simple_cmd_exec_with_ret(MMC_WRITE_BLOCK, block, &ret))
        return false;
    return dat_write();
}

#define DIVIDER 266 //266 - 375 KHz, 133 - 750 KHz

void start_mmc()
{
    // clocks
    pio_sm_config c = sd_clk_program_get_default_config(clk_pio_offset);
    sm_config_set_clkdiv_int_frac(&c, DIVIDER, 0); // 375 KHz
    sm_config_set_sideset_pins (&c, PIN_CLK); // to keep CPU in reset but not wdog
    sm_config_set_set_pins (&c, PIN_RST, 1); // SD clock
    pio_sm_init(pio0, SM_CLK, clk_pio_offset, &c);
    pio_sm_set_consecutive_pindirs (pio0, SM_CLK, PIN_CLK, 1, true);

    // CMD sniffer
    c = in_cmd_or_dat_program_get_default_config(sdin_pio_offset);
    sm_config_set_clkdiv_int_frac(&c, DIVIDER, 0);
    sm_config_set_in_pins (&c, PIN_CMD);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_jmp_pin (&c, PIN_CMD);
    pio_sm_init(pio0, SM_CMDIN, sdin_pio_offset, &c);

    // DAT sniffer
    c = in_cmd_or_dat_program_get_default_config(sdin_pio_offset);
    sm_config_set_clkdiv_int_frac(&c, DIVIDER, 0);
    sm_config_set_in_pins (&c, PIN_DAT);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_jmp_pin (&c, PIN_DAT);
    pio_sm_init(pio0, SM_DATIN, sdin_pio_offset, &c);

    // CMD writer
    c = out_cmd_or_dat_program_get_default_config(sdout_pio_offset);
    sm_config_set_clkdiv_int_frac(&c, DIVIDER, 0);
    sm_config_set_out_pins (&c, PIN_CMD, 1);
    sm_config_set_in_pins (&c, PIN_CLK);
    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_jmp_pin (&c, PIN_CLK);
    pio_sm_init(pio0, SM_OUT, sdout_pio_offset, &c);

    // put CPU into reset state
    reset_cpu();

    // enable SMs synced
    pio_enable_sm_mask_in_sync(pio0, 0xF);

    // configure pins
    for (int i = 0; i < 32; i++)
    {
        if (i == PIN_CLK || i == PIN_RST || i == PIN_CMD || (i == PIN_DAT))
        {
            gpio_init(i);
            gpio_pull_up(i);
            gpio_set_input_hysteresis_enabled(i, true);
            gpio_set_slew_rate(i, GPIO_SLEW_RATE_FAST);
            gpio_set_drive_strength(i, GPIO_DRIVE_STRENGTH_2MA);
            pio_gpio_init(pio0, i);
        }
    }
    gpio_enable_input_output(PIN_RST);
}

void stop_mmc()
{
    pio_set_sm_mask_enabled(pio0, 0xF, false);
    for (int i = 0; i < 32; i++)
    {
        if (i == PIN_CLK || i == PIN_RST || i == PIN_CMD || (i == PIN_DAT))
        {
            gpio_deinit(i);
            gpio_set_input_hysteresis_enabled(i, false);
            gpio_disable_pulls(i);
        }
    }
    gpio_disable_input_output(PIN_RST);
}

bool init_op_cond() {
    uint32_t res = 0;
    simple_cmd_exec(MMC_GO_IDLE_STATE, 0);
    if (!simple_cmd_exec_with_ret(MMC_SEND_OP_COND, 0, &res))
        return false;
    int retry = 100;
    while(--retry > 0) {
        if (!simple_cmd_exec_with_ret(MMC_SEND_OP_COND, SD_OCR_CCS | SD_OCR_VDD_18, &res))
            return false;
        if ((res & 0xFF000000) == (MMC_CARD_BUSY | SD_OCR_CCS))
            return true;
        sleep_ms(1);
    }
    return false;
}

uint32_t mmc_init_table[] = {
    MMC_SET_RELATIVE_ADDR, 2 << 16, 0x1E00, R1_STATE_IDENT << 9,
    MMC_SELECT_CARD, 2 << 16, 0x1E00, (R1_STATE_IDENT | R1_STATE_READY) << 9,
    MMC_SEND_STATUS, 2 << 16, 0x1E00, R1_STATE_TRAN << 9,
    MMC_SET_BLOCKLEN, 512, 0x1E00, R1_STATE_TRAN << 9,
    MMC_SWITCH, (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (EXT_CSD_PART_CONFIG << 16) | (1 << 8), 0x1E00, R1_STATE_TRAN << 9
};

bool mmc_initialize() {
	if (!init_op_cond()) {
        return false;
    }
    if (!cmd_exec_cid()) {
        return false;
    }
	for (int i = 0; i < sizeof(mmc_init_table)/sizeof(mmc_init_table[0]); i += 4)
    {
        uint32_t res= 0;
        if (!simple_cmd_exec_with_ret(mmc_init_table[i], mmc_init_table[i+1], &res)) {
            return false;
        }
        if (res & mmc_init_table[i+2] != mmc_init_table[i+3]) {
            return false;
        }
    }
    for(int i = 0; i < 100; i++) {
        sleep_ms(1);
        if (gpio_get(PIN_DAT))
            return true;
    }
    return false;
}

void reinit_mmc() {
    bool result = true;
    for (int i = 0; i <= 9; i++) {
        result = mmc_initialize();
        if (result) {
            break;
        }
    }
    if (!result)
    {
        halt_with_error(8, 4);
    }
}

bool is_space_bl = false;
bool is_command = false;

void write_data(int block, const uint8_t * data, int size) {
    for (int i = 0; i < size; i += 512) {
        bool was_success = false;
        bool copy_done = false;
        bool write_done = false;
        for (int j = 0; j <= 5; j++) {
            if (j == 3) {
                reinit_mmc();
                write_done = false;
            }
            if (!copy_done) {
                memcpy(data_buf, data + i, 512);
                copy_done = true;
            }
            if (!write_done) {
                if (!cmd_mmc_write(block + i / 512))
                    continue;
                write_done = true;
            }
            if (!cmd_mmc_read(block + i / 512))
                continue;
            if (memcmp(data_buf, data + i, 512) == 0) {
                was_success = true;
                break;
            }
            write_done = false;
        }
        if (!was_success)
        {
            if (write_done)
                halt_with_error(9, 4);
            else
                halt_with_error(10, 4);
        }
    }
}

extern int boot_slot;
uint8_t temp_buf[512];

struct fw_header {
	uint32_t size;
	uint32_t crc;
	uint8_t data[];
};

#define fw_slot_0 ((struct fw_header *) (XIP_BASE + 0x10000))
#define fw_slot_1 ((struct fw_header *) (XIP_BASE + 0x48000))

bool do_burn_fuses = false;

bool update_firmware(uint32_t start_block, uint32_t size_blocks) {
    int update_slot = boot_slot ^ 1;
    int fw_offset = update_slot == 0 ? 0x10000 : 0x48000;
    bool valid = true;
    bool rollback = false;
    memset(temp_buf, 0, 256);
    *(uint32_t*)temp_buf = 0x515205c5;
    write_data(1, temp_buf, 512);
    // input validation
    if (start_block == 0xFFFFFFFF && size_blocks == 0xFFFFFFFF) {
        return true;
    } else if (start_block + size_blocks > 0x1FFF) {
        return false;
    }
    else if (size_blocks > 0x1c0 || start_block > 0x1FFF) {
        return false;
    }
    // write the new firmware
    for (int b = start_block; b < start_block + size_blocks; b++)
    {
        if (!cmd_mmc_read(b) && !cmd_mmc_read(b)) {
            halt_with_error(12, 4);
        }
        if (b == start_block)
        {
            struct fw_header * new_fw = (struct fw_header *)data_buf;
            struct fw_header * cur_fw = boot_slot ? fw_slot_1 : fw_slot_0;
            if (cur_fw->crc == new_fw->crc) // do not allow to write the same firmware twice
            {
                return false;
            }
        }
        int off = fw_offset + (b - start_block) * 0x200;
        if (off % 0x1000 == 0)
            flash_range_erase(off, 0x1000);
        flash_range_program(off, data_buf, 0x200);
    }
    return true;
}
bool was_self_reset = false;
extern int boot_try;
bool fast_check() {
    start_mmc();
    reinit_mmc();
    if (!cmd_mmc_read(1) && !cmd_mmc_read(1)) {
        halt_with_error(11, 4);
    }
    if (*(uint32_t*)(data_buf + 0x20) == (mariko ? 0xA56CA203 : 0x69696969)) {
        is_space_bl = true;
    }
    if (*(uint32_t*)(data_buf) == 0x515205c5) { // chip reset
        is_command = true;
    }
    if (*(uint32_t*)(data_buf) == 0x6db92148) { // firmware update
        is_command = true;
        if (boot_try == 0) {
            // do the firmware update
            if (update_firmware(*(uint32_t*)(data_buf + 4), *(uint32_t*)(data_buf + 8))) {
                // stop mmc subsystem
                stop_mmc();
                // go to the bootloader and load new firmware
                finish_pins_except_leds();
                watchdog_enable(0, false);
                while(1);
            }
        }
    }
    // regulator i2c
    {
        const static char i2c_sda[] = {0x81, 0xFF, 0x7F, 0x00, 0xF8, 0x01, 0xE0, 0xE1, 0x1F, 0x7E, 0x00, 0x00, 0x00, 0x80, 0x9F};
        const static char i2c_scl[] = {0x33, 0x33, 0x33, 0x33, 0x33, 0xCC, 0xCC, 0xCC, 0xCC, 0x0C, 0x33, 0x33, 0x33, 0x33, 0xC3};
        gpio_pull_up(sda_pin()); // sda
        gpio_pull_up(scl_pin()); // scl
        gpio_init(sda_pin());
        gpio_init(scl_pin());
        for (int i = 0; i < sizeof(i2c_sda); i++)
        {
            for (int j = 0; j < 8; j++) 
            {
                bool sda_bit = i2c_sda[i] & (1 << j);
                bool scl_bit = i2c_scl[i] & (1 << j);
                gpio_set_dir(sda_pin(), !sda_bit);
                gpio_set_dir(scl_pin(), !scl_bit);
                sleep_us(1);
            }
        }
        gpio_disable_pulls(sda_pin());
        gpio_disable_pulls(scl_pin());
        gpio_deinit(sda_pin());
        gpio_deinit(scl_pin());
    }
    stop_mmc();
    return is_space_bl;
}

void copy_bct(int start, int end) {
    for (int i = 0; i < 0x2800; i += 0x200) {
        bool was_success = false;
        bool was_read = false;
        bool was_write = false;
        for (int k = 0; k <= 5; k++) {
            if (k == 3) {
                reinit_mmc();
                was_read = false;
                was_write = false;
            }
            if (!was_read) {
                if (!cmd_mmc_read(start + i / 512))
                    continue;
                was_read = true;
                memcpy(temp_buf, data_buf, 512);
            }
            if (!was_write) {
                if (!cmd_mmc_write(end + i / 512))
                    continue;
                was_write = true;
            }
            if (!cmd_mmc_read(end + i / 512))
                continue;
            if (memcmp(data_buf, temp_buf, 512) == 0) {
                was_success = true;
                break;
            }
            was_read = false;
            was_write = false;
        }
        if (!was_success) {
            if (was_read && !was_write)
                halt_with_error(13, 4);
            else if (was_write)
                halt_with_error(14, 4);
            else
                halt_with_error(15, 4);
        }
    }
}

struct fw_info
{
    uint32_t signature;
    uint32_t fw_major;
    uint32_t fw_minor;
    uint32_t sdloader_hash;
    uint32_t firmware_hash;
    uint32_t fuse_count;
};

extern bool do_burn_fuses;

void write_descriptor()
{
    int desc_block = 0x1FFF;
    // prepare firmware information
    memset(temp_buf, 0, 512);
    struct fw_info * fwi = (struct fw_info*)temp_buf;
    fwi->signature = 0x9cabe959;
    fwi->fuse_count = count_fuses();
    if((fwi->fuse_count & 1) != boot_slot)
        fwi->fuse_count += 1;
    fwi->fw_major = VER_HI;
    fwi->fw_minor = VER_LO;
    fwi->sdloader_hash = payload_crc();
    fwi->firmware_hash = boot_slot ? fw_slot_1->crc : fw_slot_0->crc;
    // write the info block
    write_data(desc_block, temp_buf, 512);
}

unsigned char data_bct[0x2800];

void prepare_erista_bct()
{
    memset(data_bct, 0, 0x2800);
    data_bct[1] = 2;
    data_bct[4] = 0xF;
    data_bct[5] = 0xe;
    data_bct[0x210] = 0x59;
    memset(data_bct + 0x211, 0x69, 0xFE);
    data_bct[0x30F] = 0xFF;
    memcpy(data_bct + 0x320, erista_bct_sign, 0x100);
    memcpy(data_bct + 0x232C, erista_bct_sd_sign, 0x130);
    data_bct[0x530] = 0x1;
    data_bct[0x53F] = 0x1;
    data_bct[0x540] = 0x1;
    data_bct[0x532] = 0x21;
    data_bct[0x534] = 0x0E;
    data_bct[0x544] = 0x04;
    data_bct[0x538] = 0x09;
    data_bct[0x548] = 0x09;
    data_bct[0x54C] = 0x02;
    data_bct[0x27EC] = 0x80;
}

void prepare_mariko_bct()
{
    memset(data_bct, 0, 0x2800);
    data_bct[1] = 8;
    data_bct[0x10] = 0x59;
    memset(data_bct + 0x11, 0x69, 0xFE);
    data_bct[0x10F] = 0xFF;
    memcpy(data_bct + 0x220, mariko_bct_sign, 0x100);
    memcpy(data_bct + 0x480, mariko_bct_data, 0x2380);
}

void write_payload() {
    static bool prepared = false;
    if (!prepared)
    {
        if (!mariko)
            prepare_erista_bct();
        else
            prepare_mariko_bct();
        prepared = true;
    }
    start_mmc();
    reinit_mmc();
    if (!is_space_bl && !is_command)
    {
        copy_bct(0x0, 0x7A0);
        copy_bct(0x20, 0x7C0);
    }
    write_data(0x1F80, payload, sizeof(payload));
    write_data(0x0, data_bct, 0x2800);
    write_data(0x20, data_bct, 0x2800);
    write_descriptor();
    stop_mmc();
    if (!is_space_bl && !is_command && !was_self_reset)
        halt_with_error(0, 0);
}
