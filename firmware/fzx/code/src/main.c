/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 * 		   Xiangfu@canaan-creative.com
 * 		   fanzixiao@canaan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include <string.h>
#ifdef __CODE_RED
#include <NXP/crp.h>
#endif

#include "board.h"

#include "crc.h"
#include "defines.h"
#include "libfunctions.h"
#include "protocol.h"
#include "avalon_wdt.h"
#include "avalon_timer.h"
#include "avalon_adc.h"
#include "avalon_uart.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#define STATE_NORMAL	0
#define STATE_IDLE	1

static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];

static int init_mm_pkg(struct avalon_pkg *pkg, uint8_t type)
{
	uint16_t crc;

	pkg->head[0] = AVAM_H1;
	pkg->head[1] = AVAM_H2;
	pkg->type = type;
	pkg->opt = 0;
	pkg->idx = 1;
	pkg->cnt = 1;

	crc = crc16(pkg->data, AVAM_P_DATA_LEN);
	pkg->crc[0] = (crc & 0xff00) >> 8;
	pkg->crc[1] = crc & 0x00ff;
	return 0;
}

static void process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc) {
		return;
	}

	timer_set(TIMER_ID1, IDLE_TIME);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		break;
	case AVAM_P_POLLING:
		break;
	case AVAM_P_SET_VOLT:
		break;
	default:
		break;
	}
}

static void clkout_enable(void)
{
	/* enable clk out ,use P0.1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
}

static void test_function(void)
{
	uint8_t tmp_buf[5];

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 7, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 2);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 1);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, 0);

	clkout_enable();

	while (1) {
		tmp_buf[0] = 0x0a;

		if (tmp_buf[0] & 0x01)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 1);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 0);

		if (tmp_buf[0] & 0x02)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 1);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 0);

		if (tmp_buf[0] & 0x04)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 1);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 0);

		if (tmp_buf[0] & 0x08)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 1);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 0);

		delay(50);
	}

}

int main(void)
{
	Board_Init();
	SystemCoreClockUpdate();

	test_function();

	while (1) {
		;
	}
}
