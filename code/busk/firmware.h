#include "stdint.h"

struct fw_header {
	uint32_t size;
	uint32_t crc;
	uint8_t data[];
};

typedef void nopar();

unsigned int crc32 (const unsigned char *buf, int len);