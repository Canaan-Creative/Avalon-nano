/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 * 		   Xiangfu@canaan-creative.com
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

#include "app_usbd_cfg.h"
#include "hid_ucom.h"
#include "iap.h"

#include "crc.h"
#include "sha2.h"
#include "defines.h"
#include "protocol.h"
#include "avalon_a3222.h"
#include "avalon_usb.h"
#include "avalon_wdt.h"
#include "avalon_shifter.h"
#include "avalon_timer.h"
#include "avalon_led.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

#define LED_OFF_ALL	0
#define LED_IDLE	1 /* led blue */
#define LED_BUSY	2
#define LED_ERR_ON	3 /* led red */
#define LED_ERR_OFF	4
#define LED_PG_ON	5 /* led green */
#define LED_PG_OFF	6

static uint8_t g_a3222_pkg[AVAM_P_WORKLEN];
static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];
static uint32_t g_freq[ASIC_COUNT][3];
static uint16_t g_lastvoltage = ASIC_0V;
static uint32_t g_ledstat = LED_OFF_ALL;

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
	uint8_t i;
	uint32_t val[3];
	uint8_t roll_pkg[AVAM_P_WORKLEN];
	uint16_t ntime_offset;
	char dna[8];

	static int s_tmp = 0;

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return;

	switch (pkg->type) {
	case AVAM_P_DETECT:
		a3222_sw_init();
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (!iap_readserialid(dna))
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, dna, AVAM_MM_DNA_LEN);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN, AVAM_VERSION, AVAM_MM_VER_LEN);
		UNPACK32(ASIC_COUNT, g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN + AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_WORK:
		/*
		 * idx-1: midstate(32)
		 * idx-2: job_id(2) + pool_no(2) + nonce2(4) + ntime_offset(2) + reserved(12) + data(12)
		 */

		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2)
			break;

		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			ntime_offset = (g_a3222_pkg[40] << 8) | g_a3222_pkg[41];
			if (!ntime_offset) {
				a3222_push_work(g_a3222_pkg);
				s_tmp = a3222_get_works_count();
			} else {
				memcpy(roll_pkg, g_a3222_pkg, AVAM_P_WORKLEN);
				for (i = 0; i < ntime_offset; i++) {
					a3222_roll_work(roll_pkg, 1);
					a3222_push_work(roll_pkg);
				}
			}
		}
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (a3222_get_report_count()) {
			/* P_NONCE: job_id(2) + pool_no(2) + nonce2(4) + nonce(4) */
			a3222_get_report(g_ackpkg + AVAM_P_DATAOFFSET);
			g_ackpkg[37] = a3222_get_report_count();
			g_ackpkg[36] = s_tmp;
			g_ackpkg[35] = UCOM_Read_Cnt();
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_NONCE);
		} else {
			/* P_STATUS: */
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, AVAM_VERSION, AVAM_MM_VER_LEN);
			g_ackpkg[37] = a3222_get_report_count();
			g_ackpkg[36] = s_tmp;
			g_ackpkg[35] = UCOM_Read_Cnt();
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS);
		}
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_SET_FREQ:
		PACK32(pkg->data, &val[2]);
		PACK32(pkg->data + 4, &val[1]);
		PACK32(pkg->data + 8, &val[0]);

		for (i = 0; i < ASIC_COUNT; i++) {
			memcpy(g_freq[i], val, sizeof(uint32_t) * 3);
			a3222_set_freq(val, i);
		}
		break;
	case AVAM_P_SET_VOLT:
		val[0] = (pkg->data[0] << 8) | pkg->data[1];
		if(set_voltage((uint16_t)val[0]))
			a3222_reset();

		g_lastvoltage = get_voltage();
		break;
	default:
		break;
	}
}

static inline void led_ctrl(uint8_t led_op)
{
	switch(led_op) {
	case LED_OFF_ALL:
		g_ledstat = 0;
		break;
	case LED_IDLE:
		g_ledstat |= 0xff;
		break;
	case LED_BUSY:
		g_ledstat &= 0xffff00;
		break;
	case LED_ERR_ON:
		g_ledstat |= 0xff0000;
		break;
	case LED_ERR_OFF:
		g_ledstat &= 0xffff;
		break;
	case LED_PG_ON:
		g_ledstat |= 0xff00;
		break;
	case LED_PG_OFF:
		g_ledstat &= 0xff00ff;
		break;
	}
	led_rgb(g_ledstat);
}

int main(void)
{
	uint8_t i;

	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	a3222_hw_init();
	a3222_sw_init();
	shifter_init();
	timer_init();
	led_init();

	set_voltage(ASIC_0V);
	wdt_init(5);	/* 5 seconds */
	wdt_enable();
	timer_set(TIMER_ID1, IDLE_TIME);
	led_ctrl(LED_OFF_ALL);

	while (42) {
		wdt_feed();

		if (UCOM_Read_Cnt()) {
			memset(g_reqpkg, 0, AVAM_P_COUNT);

			UCOM_Read(g_reqpkg);
			process_mm_pkg((struct avalon_pkg*)g_reqpkg);

			if ((((struct avalon_pkg*)g_reqpkg)->type == AVAM_P_WORK) ||
					(((struct avalon_pkg*)g_reqpkg)->type == AVAM_P_SET_VOLT)) {
				timer_set(TIMER_ID1, IDLE_TIME);
			}
		}

		if (timer_istimeout(TIMER_ID1)) {
			/* Power off the AISC */
			set_voltage(ASIC_0V);
			led_ctrl(LED_IDLE);
			led_ctrl(LED_PG_OFF);
			continue;
		} else {
			/* power on then reset the pll */
			if (set_voltage(g_lastvoltage)) {
				a3222_reset();
				for (i = 0; i < ASIC_COUNT; i++)
					a3222_set_freq(g_freq[i], i);
			}
			led_ctrl(LED_BUSY);
			if (read_power_good())
				led_ctrl(LED_PG_ON);
			else
				led_ctrl(LED_PG_OFF);
		}

		a3222_process();

		/* Sleep until next IRQ happens */
		//__WFI(); /* FIXME: */
	}
}
