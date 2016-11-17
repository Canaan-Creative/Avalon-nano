/*
 * @brief
 *
 * @note
 * Author: Fanzixiao fanzixiao@canaan-creative.com
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
#include "sbl_iap.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#define STATE_WORK      0
#define STATE_IDLE      1

static uint8_t  g_reqpkg[AVAM_P_COUNT];
static uint8_t  g_ackpkg[AVAM_P_COUNT];
static uint16_t g_adc_val[ADC_CAPCOUNT];
static uint16_t g_adc_buf[ADC_CAPCOUNT][ADC_DATA_LEN];
static float g_adc_ratio = 1.0;
static uint8_t g_dna[AVAM_MM_DNA_LEN];

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
	unsigned int i;

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return;

	timer_set(TIMER_ID1, IDLE_TIME, NULL);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		iap_readserialid(g_dna);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET, g_dna, AVAM_MM_DNA_LEN);
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

		for (i = 0; i < PG_COUNT; i++) {
			g_ackpkg[AVAM_P_DATAOFFSET + ADC_CAPCOUNT * 2 + i * 2] = get_pg_state(i) >> 8;
			g_ackpkg[AVAM_P_DATAOFFSET + ADC_CAPCOUNT * 2 + i * 2 + 1] = get_pg_state(i) & 0xff;
		}

		for (i = 0; i < LED_COUNT; i++) {
			g_ackpkg[AVAM_P_DATAOFFSET + ADC_CAPCOUNT * 2 + PG_COUNT * 2 + i * 2] = get_led_state(i) >> 8;
			g_ackpkg[AVAM_P_DATAOFFSET + ADC_CAPCOUNT * 2 + PG_COUNT * 2 + i * 2 + 1] = get_led_state(i) & 0xff;
		}

		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_M);
		uart_write(g_ackpkg, AVAM_P_COUNT);
		break;
	case AVAM_P_SET_VOLT:
		set_voltage((pkg->data[0] << 8) | pkg->data[1]);
		break;
	case AVAM_P_SETM:
		if (!pkg->opt)
			set_led_state((pkg->data[0] << 8) | pkg->data[1]);
		else if (pkg->opt == 1)
			g_adc_ratio = ((pkg->data[0] << 24) | (pkg->data[1] << 16) | (pkg->data[2] << 8) | pkg->data[3]) * 0.001;
		break;
	default:
		break;
	}
}

static void update_adc(void)
{
	static uint16_t adc_cnt = 0;
	uint32_t adc_sum[ADC_CAPCOUNT];
	uint16_t i = 0, j = 0;

	adc_read(ADC_CHANNEL_NTC1, &g_adc_buf[0][adc_cnt]);
	adc_read(ADC_CHANNEL_NTC2, &g_adc_buf[1][adc_cnt]);
	adc_read(ADC_CHANNEL_V12V_1, &g_adc_buf[2][adc_cnt]);
	adc_read(ADC_CHANNEL_VCORE1, &g_adc_buf[4][adc_cnt]);
	adc_read(ADC_CHANNEL_VCORE2, &g_adc_buf[5][adc_cnt]);
	adc_read(ADC_CHANNEL_VBASE, &g_adc_buf[6][adc_cnt]);

	if (++adc_cnt >= ADC_DATA_LEN)
		adc_cnt = 0;

	for (i = 0; i < ADC_CAPCOUNT; i++)
		adc_sum[i] = 0;

	for (i = 0; i < ADC_DATA_LEN; i++) {
		for (j = 0; j < ADC_CAPCOUNT; j++)
		      adc_sum[j] += g_adc_buf[j][i];
	}

	for (i = 0; i < (ADC_CAPCOUNT - 1); i++)
		g_adc_val[i] = (uint16_t)((float)(adc_sum[i] / ADC_DATA_LEN) * g_adc_ratio);

	g_adc_val[i] = (uint16_t)((float)(adc_sum[i] / ADC_DATA_LEN));
}

int main(void)
{
	uint8_t stat = STATE_WORK;
	uint32_t len = 0;

	Board_Init();
	SystemCoreClockUpdate();

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
				len = uart_read(g_reqpkg, AVAM_P_COUNT);
				if (len != AVAM_P_COUNT)
					break;

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
