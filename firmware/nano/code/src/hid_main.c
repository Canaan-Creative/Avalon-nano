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

#include "hid_uart.h"
#include "avalon_a3233.h"
#include "avalon_adc.h"
#include "avalon_iic.h"
#include "avalon_led.h"
#include "avalon_timer.h"
#include "avlaon_usb.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

#define A3233_TIMER_TIMEOUT	TMR_ID1
#define A3233_TIMER_ADJFREQ	TMR_ID2

#define A3233_STAT_IDLE					1
#define A3233_STAT_WAITICA				2
#define A3233_STAT_CHKICA				3
#define A3233_STAT_PROCICA				4
#define A3233_STAT_RCVNONCE				5
#define A3233_STAT_PROTECT				6
#define A3233_STAT_MM_PROC				7

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

/* http://blockexplorer.com/block/00000000000004b64108a8e4168cfaa890d62b8c061c6b74305b7f6cb2cf9fda */
static unsigned char golden_ob[] =		"\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
static unsigned int a3233_stat = A3233_STAT_WAITICA;
static uint8_t gica_pkg[ICA_TASK_LEN];
static uint8_t gmm_reqpkg[AVAU_P_COUNT];
static uint8_t gmm_ackpkg[AVAU_P_COUNT];
static uint32_t	a3233_freqneeded = A3233_FREQ_ADJMAX;
static uint32_t	a3233_adjstat = A3233_ADJSTAT_T;
static bool	a3323_istoohot = false;

/*
 * temp [A3233_TEMP_MIN, A3233_TEMP_MAX]
 * temp raise check  A3233_ADJ_TUPCNT
 * temp down/equal check A3233_ADJ_TDOWNCNT
 * */
static void a3233_freqmonitor()
{
	static unsigned int adc_cnt = 0;
	static unsigned int temp_cnt = 0;
	static unsigned int lasttemp = 0;
	uint16_t adc_val;
	unsigned int temp;
	Bool	adjtemp = FALSE;

	if (!A3233_IsPowerEn()) {
		if (a3323_istoohot && (I2C_TemperRd() < A3233_TEMP_MIN))
			a3323_istoohot = FALSE;
		return;
	}

	switch(a3233_adjstat){
	case A3233_ADJSTAT_T:
		adc_read(ADC_CHANNEL_V_25, &adc_val);
		if (adc_val < A3233_V25_ADJMIN) {
			temp_cnt = 0;
			a3233_adjstat = A3233_ADJSTAT_V;
			return;
		}

		temp = I2C_TemperRd();
		if (!lasttemp) {
			lasttemp = temp;
			break;
		}

		adjtemp = FALSE;
		/* TODO:may be a new way to check inflection point */
		if (lasttemp < temp) {
			if (temp_cnt >= A3233_ADJ_TUPCNT) {
				temp_cnt = 0;
				adjtemp = TRUE;
			} else
				temp_cnt++;
		} else {
			if (temp_cnt >= A3233_ADJ_TDOWNCNT) {
				temp_cnt = 0;
				adjtemp = TRUE;
			} else
				temp_cnt++;
		}

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

					a3323_istoohot = TRUE;
					return;
				}
			} else if (temp < A3233_TEMP_MIN) {
				a3323_istoohot = FALSE;
				if (a3233_freqneeded <= A3233_FREQ_ADJMAX)
					a3233_freqneeded += 20;
				if (a3233_freqneeded > A3233_FREQ_ADJMAX)
					a3233_freqneeded = A3233_FREQ_ADJMAX;
			} else {
				a3323_istoohot = FALSE;
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

	pkg->head[0] = AVAU_H1;
	pkg->head[1] = AVAU_H2;
	pkg->type = type;
	pkg->opt = 0;
	pkg->idx = 1;
	pkg->cnt = 1;

	crc = crc16(pkg->data, AVAU_P_DATA_LEN);
	pkg->crc[0] = (crc & 0xff00) >> 8;
	pkg->crc[1] = crc & 0x00ff;
	return 0;
}

static unsigned int process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	int ret;

	expected_crc = (pkg->crc[1] & 0xff)
			| ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAU_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return A3233_STAT_WAITICA;

	switch (pkg->type) {
	case AVAU_P_DETECT:
		memset(gmm_ackpkg, 0, AVAU_P_COUNT);
		memcpy(gmm_ackpkg + AVAU_P_DATAOFFSET, AVAU_VERSION, AVAU_VERSION_LEN);
		init_mm_pkg((struct avalon_pkg *)gmm_ackpkg, AVAU_P_ACKDETECT);
		UCOM_Write(gmm_ackpkg, AVAU_P_COUNT);
		ret = A3233_STAT_WAITICA;
		break;
	case AVAU_P_WORK:
		if (pkg->idx > pkg->cnt) {
			ret = A3233_STAT_WAITICA;
			break;
		}

		memcpy(gica_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == pkg->cnt)
			ret = A3233_STAT_PROCICA;
		else
			ret = A3233_STAT_MM_PROC;
		break;
	default:
		ret = A3233_STAT_WAITICA;
		break;
	}

	return ret;
}

int main(void)
{
	unsigned int icarus_buflen = 0;
	unsigned char work_buf[A3233_TASK_LEN];
	unsigned char nonce_buf[A3233_NONCE_LEN];
	unsigned int nonce_buflen = 0;
	unsigned int last_freq = 0;
	uint32_t nonce_value = 0;
	bool isgoldenob = false;
	bool timestart = false;

	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	timer_init();
	led_init();
	i2c_init();
	adc_init();
	a3233_init();

	timer_set(A3233_TIMER_ADJFREQ, 5000, a3233_freqmonitor);

	while (1) {
		switch (a3233_stat) {
		case A3233_STAT_WAITICA:
			memset(gica_pkg, 0, ICA_TASK_LEN);
			icarus_buflen = hid_rxrb_cnt();
			if (icarus_buflen > 0) {
				timestart = FALSE;
				timer_kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
				break;
			}

			if (!timestart) {
				timer_set(A3233_TIMER_TIMEOUT, 50, NULL);
				timestart = TRUE;
			}

			if (timer_istimeout(A3233_TIMER_TIMEOUT)) {
				/* no data */
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_IDLE;
			}
			break;

		case A3233_STAT_IDLE:
			led_rgb(AVALON_LED_GREEN);
			icarus_buflen = hid_rxrb_cnt();
			if (icarus_buflen > 0) {
				a3233_stat = A3233_STAT_CHKICA;
			}

			if (AVALON_A3233_IsPowerEn()) {
				AVALON_A3233_PowerEn(FALSE);
			}
			break;

		case A3233_STAT_CHKICA:
			if (AVALON_A3233_IsTooHot()) {
				a3233_stat = A3233_STAT_PROTECT;
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				break;
			}

			icarus_buflen = UCOM_Read_Cnt();
			if (icarus_buflen >= AVAU_P_COUNT) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_MM_PROC;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 80, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				/* data format error */
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				UCOM_FlushRxRB();
				a3233_stat = A3233_STAT_IDLE;
			}
			break;

		case A3233_STAT_PROTECT:
			if (!AVALON_A3233_IsTooHot()) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
				break;
			}

			if (AVALON_A3233_IsPowerEn())
				AVALON_A3233_PowerEn(FALSE);

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 10000, NULL);
				AVALON_LED_Blink(AVALON_LED_RED);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 10000, NULL);
			}
			break;

		case A3233_STAT_MM_PROC:
			if (UCOM_Read_Cnt() >= AVAU_P_COUNT) {
				memset(gmm_reqpkg, 0, AVAU_P_COUNT);
				UCOM_Read(gmm_reqpkg, AVAU_P_COUNT);
				a3233_stat = process_mm_pkg((struct avalon_pkg*)gmm_reqpkg);
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 400, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_IDLE;
			}

			if (a3233_stat == A3233_STAT_PROCICA) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
			}
			break;

		case A3233_STAT_PROCICA:
			memset(work_buf, 0, A3233_TASK_LEN);

			if (!memcmp(golden_ob, gica_pkg, ICA_TASK_LEN))
				isgoldenob = TRUE;
			else
				isgoldenob = FALSE;

			data_convert(gica_pkg);
			data_pkg(gica_pkg, work_buf);

			if (!AVALON_A3233_IsPowerEn()
					|| (last_freq != AVALON_A3233_FreqNeeded())) {
				last_freq = AVALON_A3233_FreqNeeded();
				AVALON_A3233_PowerEn(TRUE);
				AVALON_A3233_Reset();
				((unsigned int*) work_buf)[1] = AVALON_A3233_PllCfg(last_freq,
				NULL);
			}

			{
				unsigned int r, g, b;
				r = last_freq - AVALON_A3233_FreqMin();
				if (r > 255)
					r = 255;
				g = 0;
				b = 255 - r;
				AVALON_LED_Rgb((r << 16) | (g << 8) | b);
			}

			if (isgoldenob) {
				work_buf[81] = 0x1;
				work_buf[82] = 0x73;
				work_buf[83] = 0xa2;
			}

			UART_Write(work_buf, A3233_TASK_LEN);
			UART_FlushRxRB();
			a3233_stat = A3233_STAT_RCVNONCE;
			break;

		case A3233_STAT_RCVNONCE:
			nonce_buflen = UART_Read_Cnt();
			if (nonce_buflen >= A3233_NONCE_LEN) {
				UART_Read(nonce_buf, A3233_NONCE_LEN);
				PACK32(nonce_buf, &nonce_value);
				nonce_value = ((nonce_value >> 24) | (nonce_value << 24)
						| ((nonce_value >> 8) & 0xff00)
						| ((nonce_value << 8) & 0xff0000));
				nonce_value -= 0x1000;
				UNPACK32(nonce_value, nonce_buf);
				memset(&gmm_ackpkg, 0, AVAU_P_COUNT);
				memcpy(gmm_ackpkg + AVAU_P_DATAOFFSET, nonce_buf, 4);
				init_mm_pkg((struct avalon_pkg *)gmm_ackpkg, AVAU_P_NONCE);
				UCOM_Write(gmm_ackpkg, AVAU_P_COUNT);
#ifdef A3233_FREQ_DEBUG
				{
					char freq[20];

					m_sprintf(freq, "%04d%04d%04d%04d", AVALON_A3233_FreqNeeded(), AVALON_I2C_TemperRd(), (int)AVALON_A3233_IsTooHot(), AVALON_A3233_ADCGuard(0));
					UCOM_Write(freq, 16);
				}
#endif
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_WAITICA;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 500, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				/* no nonce response */
				uint32_t tmp;

				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_IDLE;
				memset(&gmm_ackpkg, 0, AVAU_P_COUNT);
				tmp = AVALON_A3233_FreqNeeded();
				tmp = ((tmp >> 24) | (tmp << 24)
						| ((tmp >> 8) & 0xff00)
						| ((tmp << 8) & 0xff0000));
				memcpy(gmm_ackpkg + AVAU_P_DATAOFFSET, (uint8_t*)&tmp, 4);
				init_mm_pkg((struct avalon_pkg *)gmm_ackpkg, AVAU_P_STATUS);
				hid_write(gmm_ackpkg, AVAU_P_COUNT);
				break;
			}

			if (hid_rxrb_Cnt()) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
			}
			break;

		default:
			a3233_stat = A3233_STAT_IDLE;
			break;
		}
		/* Sleep until next IRQ happens */
		__WFI();
	}
}
