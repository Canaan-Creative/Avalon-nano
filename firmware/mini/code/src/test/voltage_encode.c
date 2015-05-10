#include <stdio.h>
#include <stdint.h>

/* http://www.onsemi.com/pub/Collateral/NCP5392P-D.PDF */
static inline uint8_t encode_voltage_ncp5392p(uint32_t v)
{
	if (v == 0)
		return 0xff;

	return (((0x59 - (v - 5000) / 125) & 0xff) << 1 | 1);
}

static inline uint32_t decode_voltage_ncp5392p(uint8_t v)
{
	if (v == 0xff)
		return 0;

	return (0x59 - (v >> 1)) * 125 + 5000;
}

int main()
{
	int v = 0;
	uint8_t encode_val;

	v = 5000;
	while (v <= 16000) {
	    encode_val = encode_voltage_ncp5392p(v);
	    printf("%d --> %02x | %d --> %d\n", v, encode_val, encode_val, decode_voltage_ncp5392p(encode_val));
	    v += 125;
	}

	encode_val = encode_voltage_ncp5392p(0);
	printf("%d --> %02x | %d --> %d\n", 0, encode_val, encode_val, decode_voltage_ncp5392p(encode_val));

	return 0;
}

