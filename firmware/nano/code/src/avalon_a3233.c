/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#include <string.h>

#include "board.h"
#include "defines.h"
#include "libfunctions.h"
#include "sha2.h"
#include "hid_uart.h"
#include "avalon_a3233.h"

#define A3233_VOLT_0P9   0x0
#define A3233_VOLT_0P8   0x1
#define A3233_VOLT_0P725 0x2
#define A3233_VOLT_0P675 0x3

/* http://blockexplorer.com/block/00000000000004b64108a8e4168cfaa890d62b8c061c6b74305b7f6cb2cf9fda */
static unsigned char golden_ob[] =		"\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
static bool	ispoweron = false;
static bool g_freqflag;
static uint32_t g_pll;

static void a3233_powerinit()
{
	/* VID0 */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);
	/* VID1 */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);
	/* VCore */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);
	/* TODO: use function to set no register op */
	*(uint32_t *) 0x4004402c = 0x81;

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, true);
}

static void a3233_setvoltage(uint8_t voltage)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, (voltage & 1));
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, (voltage >> 1) & 1);
}

#define FREF_MIN 10
#define FREF_MAX 50
#define FVCO_MIN 800
#define FVCO_MAX 1600
#define FOUT_MIN 100
#define FOUT_MAX 1600
#define WORD0_BASE 0x7
static unsigned int a3233_getpll(unsigned int freq)
{
	unsigned int nox[4], i = 0;
	unsigned int no = 0;
	unsigned int fin = 25;
	unsigned int nr = 0;
	unsigned int fvco = 0;
	unsigned int fout = 200;
	unsigned int nf = 0;
	unsigned int fref = 0;
	unsigned int od;
	unsigned int tmp;

	nox[0] = 1;
	nox[1] = 2;
	nox[2] = 4;
	nox[3] = 8;

	for (fout = freq; fout <= 1300; fout += 1) {
		for (i = 0; i < 4; i++) {
			no = nox[i];
			for (nr = 1; nr < 32; nr++) {
				for (nf = 1; nf < 128; nf++) {
					fref = fin / nr;
					fvco = fout * no;
					if ((fout == (fin * nf / (nr * no))) &&
						(FREF_MIN <= fref && fref <= FREF_MAX) &&
						(FVCO_MIN <= fvco && fvco <= FVCO_MAX) &&
						(FOUT_MIN <= fout && fout <= FOUT_MAX)) {
						if(no == 1) od = 0;
						if(no == 2) od = 1;
						if(no == 4) od = 2;
						if(no == 8) od = 3;

						tmp = WORD0_BASE |
							((nr - 1) & 0x1f) << 16 |
							((nf / 2 - 1) & 0x7f) << 21 |
							(od << 28);

						return tmp;
					}
				}
			}
		}
	}

	return 0;
}

void a3233_init(void)
{
	/* rst_n */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);

	/* init power */
	a3233_enable_power(false);
	a3233_powerinit();
	a3233_setvoltage(A3233_VOLT_0P675);
	a3233_enable_power(true);

	/* enable clk */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
}

void a3233_enable_power(bool on)
{
	ispoweron = on;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, on);
}

bool a3233_power_isenable(void)
{
	return ispoweron;
}

void a3233_reset_asic(void)
{
	delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, false);
	delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	delay(2);
}

int a3233_push_work(uint8_t *pkg)
{
	unsigned char awork[A3233_WORK_SIZE];
	bool isgoldenob = false;

	memset(awork, 0, A3233_WORK_SIZE);

	if (!memcmp(golden_ob, pkg, ICA_WORK_SIZE))
		isgoldenob = true;
	else
		isgoldenob = false;

	data_convert(pkg);
	data_pkg(pkg, awork);

	if (g_freqflag) {
		memcpy(awork + 4, &g_pll, 4);
		g_freqflag = false;
	} else
		memcpy(awork + 4, "\x0\x0\x0\x1", 4);

	if (isgoldenob) {
		awork[81] = 0x1;
		awork[82] = 0x73;
		awork[83] = 0xa2;
	}

	uart_write(awork, A3233_WORK_SIZE);
	uart_flush_rxrb();

	return 0;
}

int a3222_get_nonce(uint32_t *nonce)
{
	unsigned char nonce_buf[A3233_NONCE_SIZE];
	uint32_t nonce_value = 0;

	if (uart_rxrb_cnt() >= A3233_NONCE_SIZE) {
		uart_read(nonce_buf, A3233_NONCE_SIZE);
		PACK32(nonce_buf, &nonce_value);
		nonce_value = ((nonce_value >> 24) | (nonce_value << 24)
				| ((nonce_value >> 8) & 0xff00)
				| ((nonce_value << 8) & 0xff0000));
		nonce_value -= 0x1000;
		nonce_value += 0x4000;
		UNPACK32(nonce_value, nonce_buf);
		memcpy(nonce, &nonce_value, A3233_NONCE_SIZE);
		return 0;
	}

	return 1;
}

void a3233_set_freq(uint32_t freq)
{
	g_freqflag = true;
	g_pll = a3233_getpll(freq);
}

