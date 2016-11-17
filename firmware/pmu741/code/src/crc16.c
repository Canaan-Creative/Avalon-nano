
#include "crc.h"

static inline unsigned short shitfer(unsigned char ch, unsigned short init)
{
	unsigned char mask;
	unsigned int crc = init;

	mask = 0x80;
	while (mask > 0) {
		crc <<= 1;

		if (ch & mask)
			crc += 1;

		mask >>= 1;
		if (crc > 0xffff) {
			crc &= 0xffff;
			crc ^= 0x1021;
		}
	}

	return (unsigned short)(crc & 0xffff);
}

unsigned short crc16(const unsigned char *buffer, int len)
{
	unsigned short crc;
	unsigned char *p = (unsigned char *)buffer;

	crc = 0;
	while (len-- > 0)
		crc = shitfer(*p++, crc);

	crc = shitfer(0, crc);
	crc = shitfer(0, crc);

	return crc;
}

