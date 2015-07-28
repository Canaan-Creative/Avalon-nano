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
#include "app_usbd_cfg.h"
#ifdef __CODE_RED
#include <NXP/crp.h>
#endif
#include "crc.h"
#include "sha2.h"
#include "protocol.h"
#include "defines.h"

#include "hid_uart.h"
#include "avalon_a3233.h"
#include "avalon_adc.h"
#include "avalon_iic.h"
#include "avalon_led.h"
#include "avalon_timer.h"
#include "avalon_usb.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

#define A3233_TIMER_TIMEOUT	TIMER_ID1
#define A3233_TIMER_ADJFREQ	TIMER_ID2

#define A3233_STAT_IDLE	1
#define A3233_STAT_WORK	2

#define A3233_TEMP_MIN	60
#define A3233_TEMP_MAX	65
#define A3233_FREQ_ADJMIN	100
#define A3233_FREQ_ADJMAX	360
#define A3233_V25_ADJMIN	(int)(2.2 * 1024 / 5)
#define A3233_V25_ADJMAX	(A3233_V25_ADJMIN + 10)
#define A3233_TIMER_INTERVAL	5000
#define A3233_ADJ_VCNT	2		/* n x 5s */
#define A3233_ADJ_TUPCNT	1		/* n x 5s */
#define A3233_ADJ_TDOWNCNT	2		/* n x 5s */
#define A3233_ADJSTAT_T	0
#define A3233_ADJSTAT_V	1

static unsigned int a3233_stat = A3233_STAT_IDLE;
static uint8_t gica_pkg[ICA_WORK_SIZE];
static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];
static uint32_t	a3233_freqneeded = A3233_FREQ_ADJMAX;
static uint32_t	a3233_adjstat = A3233_ADJSTAT_T;
static bool	a3233_istoohot = false;
static uint8_t gwork_id[6];
static uint32_t g_ledstatus = LED_GREEN;

/*
 * temp [A3233_TEMP_MIN, A3233_TEMP_MAX]
 * temp raise check  A3233_ADJ_TUPCNT
 * temp down/equal check A3233_ADJ_TDOWNCNT
 * */
static void a3233_monitor()
{
	static unsigned int adc_cnt = 0;
	static unsigned int temp_cnt = 0;
	static int lasttemp = 0;
	uint16_t adc_val;
	int temp;
	Bool	adjtemp = false;

	if (!a3233_power_isenable()) {
		temp = i2c_readtemp();
		if (a3233_istoohot && (temp < A3233_TEMP_MIN))
			a3233_istoohot = false;
		return;
	}

	switch (a3233_adjstat) {
	case A3233_ADJSTAT_T:
		adc_read(ADC_CHANNEL_V_25, &adc_val);
		if (adc_val < A3233_V25_ADJMIN) {
			temp_cnt = 0;
			a3233_adjstat = A3233_ADJSTAT_V;
			return;
		}

		temp = i2c_readtemp();
		if (!lasttemp) {
			lasttemp = temp;
			break;
		}

		adjtemp = false;
		/* TODO:may be a new way to check inflection point */
		if (lasttemp < temp) {
			if (temp_cnt >= A3233_ADJ_TUPCNT) {
				temp_cnt = 0;
				adjtemp = true;
			} else
				temp_cnt++;
		} else {
			if (temp_cnt >= A3233_ADJ_TDOWNCNT) {
				temp_cnt = 0;
				adjtemp = true;
			} else
				temp_cnt++;
		}
		lasttemp = temp;

		if (adjtemp) {
			if (temp >= A3233_TEMP_MAX) {
				if (a3233_freqneeded > A3233_FREQ_ADJMIN) {
					a3233_freqneeded -= 40;
					if (a3233_freqneeded < A3233_FREQ_ADJMIN)
						a3233_freqneeded = A3233_FREQ_ADJMIN;
				} else {
					/* notify a3233 is too hot */
					if (a3233_freqneeded < A3233_FREQ_ADJMIN)
						a3233_freqneeded = A3233_FREQ_ADJMIN;

					a3233_istoohot = true;
					return;
				}
			} else if (temp < A3233_TEMP_MIN) {
				a3233_istoohot = false;
				if (a3233_freqneeded <= A3233_FREQ_ADJMAX)
					a3233_freqneeded += 20;
				if (a3233_freqneeded > A3233_FREQ_ADJMAX)
					a3233_freqneeded = A3233_FREQ_ADJMAX;
			} else {
				a3233_istoohot = false;
			}
		}
		break;

	case A3233_ADJSTAT_V:
		adc_read(ADC_CHANNEL_V_25, &adc_val);
		if (adc_cnt == A3233_ADJ_VCNT) {
			adc_cnt = 0;
			if (adc_val >= A3233_V25_ADJMIN) {
				if (adc_val > A3233_V25_ADJMAX)
					a3233_adjstat = A3233_ADJSTAT_T;
				break;
			} else {
				a3233_freqneeded -= 20;
				if (a3233_freqneeded < A3233_FREQ_ADJMIN)
					a3233_freqneeded = A3233_FREQ_ADJMIN;

				/* FIXME: if a3233_freqneeded = A3233_FREQ_ADJMIN also cann't work */
			}
		}
		adc_cnt++;
		break;
	}
}

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

static unsigned int process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	int ret;
	uint32_t nonce_value = 0;
	static unsigned int last_freq = 0;
	char dna[AVAM_MM_DNA_LEN];

	expected_crc = (pkg->crc[1] & 0xff)
			| ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return 1;

	timer_set(A3233_TIMER_TIMEOUT, IDLE_TIME, NULL);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (!iap_readserialid(dna))
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, dna, AVAM_MM_DNA_LEN);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN, AVAM_VERSION, AVAM_MM_VER_LEN);
		UNPACK32(ASIC_COUNT, g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN + AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		hid_write(g_ackpkg, AVAM_P_COUNT);
		ret = 0;
		break;
	case AVAM_P_WORK:
		if (a3233_istoohot) {
			ret = 1;
			break;
		}

		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2) {
			ret = 1;
			break;
		}

		memcpy(gica_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			memcpy(gwork_id, gica_pkg + 32, 6);
			if (!a3233_power_isenable()
					|| (last_freq != a3233_freqneeded)) {
				last_freq = a3233_freqneeded;
				a3233_enable_power(true);
				a3233_reset_asic();
				a3233_set_freq(last_freq);
			}

			{
				unsigned int r, g, b;
				r = last_freq - A3233_FREQ_ADJMIN;
				if (r > 255)
					r = 255;
				g = 0;
				b = 255 - r;
				g_ledstatus = (r << 16) | (g << 8) | b;
				led_rgb(g_ledstatus);
			}
			a3233_push_work(gica_pkg);
		}

		ret = 0;
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0xff, AVAM_P_COUNT);
		if (!a3222_get_nonce(&nonce_value)) {
			/* P_NONCE: id(6) + chip_id(1) + ntime(1) + nonce(4) + reserved(1) + usb rb(1) + work rb(1) + nonce rb(1) */
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, gwork_id, 6);
			g_ackpkg[AVAM_P_DATAOFFSET + 6] = 0;
			g_ackpkg[AVAM_P_DATAOFFSET + 7] = 0;
			UNPACK32(nonce_value,  g_ackpkg + AVAM_P_DATAOFFSET + 8);
			g_ackpkg[AVAM_P_DATAOFFSET + 12] = 0;
			g_ackpkg[AVAM_P_DATAOFFSET + 13] = 0;
			g_ackpkg[AVAM_P_DATAOFFSET + 14] = 0;
			g_ackpkg[AVAM_P_DATAOFFSET + 15] = 0;
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_NONCE_M);
		} else {
			/* P_STATUS_M: spi speed(4) + led(4) + fan(4) + voltage(4) + frequency(12) + power good(4) */
			UNPACK32(0, g_ackpkg + AVAM_P_DATAOFFSET);
			UNPACK32(g_ledstatus, g_ackpkg + AVAM_P_DATAOFFSET + 4);
			UNPACK32((uint32_t)-1, g_ackpkg + AVAM_P_DATAOFFSET + 8);
			UNPACK32(3, g_ackpkg + AVAM_P_DATAOFFSET + 12);
			/* TODO: temp + adc x 2 */

			UNPACK32(0, g_ackpkg + AVAM_P_DATAOFFSET + 16);
			UNPACK32(i2c_readtemp(), g_ackpkg + AVAM_P_DATAOFFSET + 20);
			UNPACK32(0, g_ackpkg + AVAM_P_DATAOFFSET + 24);
			UNPACK32(a3233_power_isenable(), g_ackpkg + AVAM_P_DATAOFFSET + 28);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_M);
		}
		hid_write(g_ackpkg, AVAM_P_COUNT);
		break;
	default:
		ret = 1;
		break;
	}

	return ret;
}

int main(void)
{
	unsigned int icarus_buflen = 0;
	bool timestart = false;

	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	timer_init();
	led_init();
	i2c_init();
	adc_init();
	a3233_init();

	timer_set(A3233_TIMER_ADJFREQ, 5000, a3233_monitor);
	timer_set(A3233_TIMER_TIMEOUT, IDLE_TIME, NULL);

	while (1) {
		switch (a3233_stat) {
		case A3233_STAT_WORK:
			icarus_buflen = hid_rxrb_cnt();
			if (icarus_buflen >= AVAM_P_COUNT) {
				memset(g_reqpkg, 0, AVAM_P_COUNT);
				hid_read(g_reqpkg, AVAM_P_COUNT);
				process_mm_pkg((struct avalon_pkg*)g_reqpkg);
			}

			if (timer_istimeout(A3233_TIMER_TIMEOUT))
				a3233_stat = A3233_STAT_IDLE;

			/* NOTE: protect is high priority than others */
			if (a3233_istoohot) {
				if (a3233_power_isenable()) {
					a3233_enable_power(false);
					g_ledstatus = LED_RED;
					led_blink(g_ledstatus);
				}
			}
		break;
		case A3233_STAT_IDLE:
			g_ledstatus = LED_GREEN;
			led_rgb(g_ledstatus);
			icarus_buflen = hid_rxrb_cnt();
			if (icarus_buflen >= AVAM_P_COUNT)
				a3233_stat = A3233_STAT_WORK;

			if (a3233_power_isenable())
				a3233_enable_power(false);
			__WFI();
			break;
		default:
			a3233_stat = A3233_STAT_IDLE;
			break;
		}
	}
}

