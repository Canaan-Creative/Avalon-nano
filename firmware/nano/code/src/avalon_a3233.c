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

#include "board.h"
#include "avalon_a3233.h"

#define A3233_VOLT_0P9   0x0
#define A3233_VOLT_0P8   0x1
#define A3233_VOLT_0P725 0x2
#define A3233_VOLT_0P675 0x3

static bool	ispoweron = false;

static void a3233_poweren(bool on)
{
	ispoweron = on;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, on);
}

static bool a3233_ispoweren(void)
{
	return ispoweron;
}

static void a3233_powerinit()
{
	/* VID0 */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);
	/* VID1 */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);
	/* VCore */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);
	/* TodO: use function to set no register op */
	*(uint32_t *) 0x4004402c = 0x81;

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, true);
}

static void a3233_setvoltage(uint8_t voltage)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, (voltage & 1));
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, (voltage >> 1) & 1);
}

static void a3233_resetn(void)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
}

void a3233_init(void)
{
	/* rst_n */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);

	/* init power */
	a3233_poweren(false);
	a3233_powerinit();
	a3233_setvoltage(A3233_VOLT_0P675);
	a3233_poweren(true);

	/* enable clk */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);

}

void a3233_asicreset(void)
{
	Delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	Delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, false);
	Delay(2);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
	Delay(2);
}

#define FREF_MIN 10
#define FREF_MAX 50
#define FVCO_MIN 800
#define FVCO_MAX 1600
#define FOUT_MIN 100
#define FOUT_MAX 1600
#define WORD0_BASE 0x7
unsigned int a3233_getpll(unsigned int freq)
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

