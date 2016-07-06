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
#include "avalon_vcore.h"
#include "avalon_led.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#define STATE_WORK      0
#define STATE_IDLE      1

static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];
static uint8_t g_adc_val[ADC_CAPCOUNT];

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

static void power_detect(struct avalon_pkg *pkg)
{
	switch (pkg->data[0]) {
	case POWER_ON:
		if (pkg->data[1] & 0x03)
			set_voltage(pkg->data[2]);

		if (pkg->data[1] & 0x01)
			vcore_enable(VCORE1);

		if (pkg->data[1] & 0x02)
			vcore_enable(VCORE2);
		led_blink_on(LED_12V);
		break;
	case POWER_OFF:
		if (pkg->data[1] & 0x01)
			vcore_disable(VCORE1);

		if (pkg->data[1] & 0x02)
			vcore_disable(VCORE2);
		break;
	case POWER_ERR:
		if (pkg->data[1] & 0x01)
			led_blink_on(LED_12V_1T);

		if (pkg->data[1] & 0x02)
			led_blink_on(LED_12V_2T);
		break;
	case POWER_OK:
		if (pkg->data[1] & 0x01)
			led_blink_off(LED_12V_1T);

		if (pkg->data[1] & 0x02)
			led_blink_off(LED_12V_2T);
		break;
	default:
		break;
	}
}

static void process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	unsigned int i;

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc) {
		return;
	}

	timer_set(TIMER_ID1, IDLE_TIME, NULL);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN, AVAM_VERSION, AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		uart_write(g_ackpkg, AVAM_P_COUNT);
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0, AVAM_P_COUNT);

		for (i = 0; i < ADC_CAPCOUNT; i++) {
			g_ackpkg[AVAM_P_DATAOFFSET + i * 2] = g_adc_val[i] >> 8;
			g_ackpkg[AVAM_P_DATAOFFSET + i * 2 + 1] = g_adc_val[i] & 0xff;
		}
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_M);
		uart_write(g_ackpkg, AVAM_P_COUNT);
		break;
	case AVAM_P_SET_VOLT:
		set_voltage(pkg->data[0]);
		vcore_detect();
		break;
	case AVAM_P_TEST:
		power_detect(pkg);
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

static void update_adc(void)
{
	adc_read(ADC_CHANNEL_NTC1, &g_adc_val[0]);
	adc_read(ADC_CHANNEL_NTC2, &g_adc_val[1]);
	adc_read(ADC_CHANNEL_V12V_1, &g_adc_val[2]);
	adc_read(ADC_CHANNEL_V12V_2, &g_adc_val[3]);
	adc_read(ADC_CHANNEL_VCORE1, &g_adc_val[4]);
	adc_read(ADC_CHANNEL_VCORE2, &g_adc_val[5]);
}

int main(void)
{
	uint8_t stat = STATE_WORK;
	uint32_t len = 0;

	Board_Init();
	SystemCoreClockUpdate();

	clkout_enable();
	timer_init();
	led_init();
	adc_init();
	uart_init();
	vcore_init();

	timer_set(TIMER_ID1, IDLE_TIME, NULL);
	timer_set(TIMER_ID2, ADC_CAPTIME, NULL);
	while (1) {
		switch (stat) {
		case STATE_WORK:
			len = uart_rxrb_cnt();
			if (len >= AVAM_P_COUNT) {
				memset(g_reqpkg, 0, AVAM_P_COUNT);
				uart_read(g_reqpkg, AVAM_P_COUNT);
				process_mm_pkg((struct avalon_pkg*)g_reqpkg);
			}

			if (timer_istimeout(TIMER_ID1))
				stat = STATE_IDLE;
			break;
		case STATE_IDLE:
			len = uart_rxrb_cnt();
			if (len >= AVAM_P_COUNT)
				stat = STATE_WORK;

			__WFI();
			break;
		default:
			stat = STATE_IDLE;
			break;
		}

		if (timer_istimeout(TIMER_ID2)) {
			update_adc();
			timer_set(TIMER_ID2, ADC_CAPTIME, NULL);
		}
	}
}
