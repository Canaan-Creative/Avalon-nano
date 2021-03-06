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
#include "sbl_iap.h"
#include "dfu.h"

#include "crc.h"
#include "sha2.h"
#include "defines.h"
#include "libfunctions.h"
#include "protocol.h"
#include "avalon_a3222.h"
#include "avalon_usb.h"
#include "avalon_wdt.h"
#include "avalon_shifter.h"
#include "avalon_timer.h"
#include "avalon_led.h"
#include "avalon_adc.h"
#include "avalon_uart.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#define LED_OFF_ALL	0
#define LED_IDLE	1 /* led blue */
#define LED_BUSY	2
#define LED_ERR_ON	3 /* led red */
#define LED_ERR_OFF	4
#define LED_PG_ON	5 /* led green */
#define LED_PG_OFF	6
#define LED_ON_ALL	7
#define LED_HOT_ALERT	8

#define STATE_NORMAL	0
#define STATE_IDLE	1

static uint8_t g_a3222_pkg[AVAM_P_WORKLEN];
static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];
static uint32_t g_freq[ASIC_COUNT][3];
static uint32_t g_ledstat = LED_OFF_ALL;
static bool g_toohot = false;

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

static void led_ctrl(uint8_t led_op)
{
	switch(led_op) {
	case LED_OFF_ALL:
		g_ledstat = 0;
		led_set(LED_RED, LED_OFF);
		led_set(LED_GREEN, LED_OFF);
		led_set(LED_BLUE, LED_OFF);
		break;
	case LED_IDLE:
		g_ledstat |= 0x10000ff;
		led_set(LED_BLUE, LED_BREATH);
		break;
	case LED_BUSY:
		g_ledstat &= 0xffff00;
		led_set(LED_BLUE, LED_OFF);
		break;
	case LED_ERR_ON:
		g_ledstat |= 0xff0000;
		led_set(LED_RED, LED_ON);
		break;
	case LED_ERR_OFF:
		g_ledstat &= 0xffff;
		led_set(LED_RED, LED_OFF);
		break;
	case LED_PG_ON:
		g_ledstat |= 0xff00;
		led_set(LED_GREEN, LED_ON);
		break;
	case LED_PG_OFF:
		g_ledstat &= 0xff00ff;
		led_set(LED_GREEN, LED_OFF);
		break;
	case LED_ON_ALL:
		g_ledstat = 0xffffff;
		led_set(LED_RED, LED_ON);
		led_set(LED_GREEN, LED_ON);
		led_set(LED_BLUE, LED_ON);
		break;
	case LED_HOT_ALERT:
		g_ledstat |= 0x200ff00;
		led_set(LED_GREEN, LED_BREATH);
		break;
	}
}

static unsigned int testcores(uint32_t core_num, uint32_t ret)
{
	uint32_t result[ASIC_COUNT];
	uint8_t txdat[20];
	uint32_t all = ASIC_COUNT * core_num;
	uint32_t pass_cal_num, pass[ASIC_COUNT], i, j;
	uint8_t golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
	uint8_t	report[A3222_REPORT_SIZE];

	pass_cal_num = 0;
	memset(result, 0, ASIC_COUNT * sizeof(uint32_t));

	for (i = 0; i < (core_num + 2); i++) {
		wdt_feed();
#ifdef DEBUG_VERBOSE
		debug32("D: core %d\n", i);
#endif
		for (j = 0; j < ASIC_COUNT; j++) {
			pass[j] = 0;
			a3222_push_work(golden_ob);
		}

		a3222_process();
		delay(40);
		if (i >= 2) {
			while (a3222_get_report_count()) {
				a3222_get_report(report);
				if (0x0001c7a2 == (report[8] << 24 |
						report[9] << 16 |
						report[10] << 8 |
						report[11])) {
					pass[report[6]] = 1;
				} else {
					debug32("D: N = %x, C = %d\n", report[8] << 24 |
						report[9] << 16 |
						report[10] << 8 |
						report[11], report[6]);
				}
			}

			for (j = 0; j < ASIC_COUNT; j++) {
				if (pass[j])
					pass_cal_num++;
				else
					result[j]++;
			}
		}
	}

	txdat[0] = 0;
	for (i = 0; i < ASIC_COUNT; i++) {
		txdat[1 + (i % 4) * 4] = (result[i] >> 24) & 0xff;
		txdat[2 + (i % 4) * 4] = (result[i] >> 16) & 0xff;
		txdat[3 + (i % 4) * 4] = (result[i] >> 8) & 0xff;
		txdat[4 + (i % 4) * 4] = result[i] & 0xff;
		debug32("%d ", result[i]);

		if (ret && !((i + 1) % 4)) {
			memset(g_ackpkg, 0, AVAM_P_COUNT);
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, txdat, 4 * 4 + 1);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_TEST_RET);
			UCOM_Write(g_ackpkg);
		}
	}

	/* send left */
	if (ret && (i % 4)) {
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET, txdat, 4 * 4 + 1);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_TEST_RET);
		UCOM_Write(g_ackpkg);
	}

	if (ret) {
		txdat[0] = (pass_cal_num >> 24) & 0xff;
		txdat[1] = (pass_cal_num >> 16) & 0xff;
		txdat[2] = (pass_cal_num >> 8) & 0xff;
		txdat[3] = pass_cal_num & 0xff;
		txdat[4] = (all >> 24) & 0xff;
		txdat[5] = (all >> 16) & 0xff;
		txdat[6] = (all >> 8) & 0xff;
		txdat[7] = all & 0xff;
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET, txdat, 8);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_TEST_RET);
		UCOM_Write(g_ackpkg);
	}

	debug32("E/A: %d/%d\n", all - pass_cal_num, all);
	return all - pass_cal_num;
}

static void process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	uint8_t i;
	uint32_t val[3], test_core_count;
	uint8_t roll_pkg[AVAM_P_WORKLEN];
	uint16_t ntime_offset, adc_val;
	char dna[8];

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc) {
		debug32("E: crc err (%x-%x)\n", expected_crc, actual_crc);
		return;
	}

	timer_set(TIMER_ID1, IDLE_TIME);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		set_voltage(ASIC_0V);
		a3222_sw_init();
		UCOM_Flush();

		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (!iap_readserialid(dna))
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, dna, AVAM_MM_DNA_LEN);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN, AVAM_VERSION, AVAM_MM_VER_LEN);
		UNPACK32(ASIC_COUNT, g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN + AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_WORK:
		if (g_toohot)
			break;
		/*
		 * idx-1: midstate(32)
		 * idx-2: id(6) + reserved(2) + ntime(1) + fan(3) + led(4) + reserved(4) + data(12)
		 */
		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2)
			break;

		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			ntime_offset = pkg->data[8];
			if (!ntime_offset) {
				a3222_set_ntime(0);
				a3222_push_work(g_a3222_pkg);
			} else {
				memcpy(roll_pkg, g_a3222_pkg, AVAM_P_WORKLEN);
				for (i = 0; i <= ntime_offset; i++) {
					a3222_set_ntime(i);
					a3222_push_work(roll_pkg);
					a3222_roll_work(roll_pkg, 1);
				}
			}
		}
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0xff, AVAM_P_COUNT);
		if (a3222_get_report_count()) {
			/* P_NONCE: id(6) + chip_id(1) + ntime(1) + nonce(4) + reserved(1) + usb rb(1) + work rb(1) + nonce rb(1) */
			val[0] = a3222_get_works_count();
			a3222_get_report(g_ackpkg + AVAM_P_DATAOFFSET);
			g_ackpkg[AVAM_P_DATAOFFSET + 13] = UCOM_Read_Cnt();
			g_ackpkg[AVAM_P_DATAOFFSET + 14] = (uint8_t)val[0];
			g_ackpkg[AVAM_P_DATAOFFSET + 15] = a3222_get_report_count();
			if (a3222_get_report_count()) {
				val[0] = a3222_get_works_count();
				a3222_get_report(g_ackpkg + AVAM_P_DATAOFFSET + 16);
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 13] = UCOM_Read_Cnt();
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 14] = (uint8_t)val[0];
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 15] = a3222_get_report_count();
			}

			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_NONCE_M);
		} else {
			/* P_STATUS_M: spi speed(4) + led(4) + fan(4) + voltage(4) + frequency(12) + power good(4) */
			val[0] = a3222_get_spispeed();
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET);
			val[0] = g_ledstat;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 4);
			val[0] = (uint32_t)-1;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 8);
			val[0] = get_voltage();
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 12);
			adc_read(ADC_CHANNEL_12V, &adc_val);
			val[0] = adc_val;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 16);
			adc_read(ADC_CHANNEL_COPPER, &adc_val);
			val[0] = adc_val;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 20);
			adc_read(ADC_CHANNEL_FAN, &adc_val);
			val[0] = adc_val;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 24);
			val[0] = read_power_good();
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 28);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_M);
		}
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_SET_FREQ:
		if (g_toohot)
			break;

		PACK32(pkg->data, &val[2]);
		PACK32(pkg->data + 4, &val[1]);
		PACK32(pkg->data + 8, &val[0]);

		debug32("D: (%d)-F(%x, %x, %x)\n", pkg->opt, val[2], val[1], val[0]);
		if (!pkg->opt) {
			for (i = 0; i < ASIC_COUNT; i++) {
				memcpy(g_freq[i], val, sizeof(uint32_t) * 3);
				a3222_set_freq(val, i);
			}
		}

		if (pkg->opt) {
			memcpy(g_freq[pkg->opt - 1], val, sizeof(uint32_t) * 3);
			a3222_set_freq(val, pkg->opt - 1);
		}
		break;

	case AVAM_P_GET_FREQ:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (!pkg->opt) {
			val[0] = g_freq[0][2];
			val[1] = g_freq[0][1];
			val[2] = g_freq[0][0];

			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET);
			UNPACK32(val[1], g_ackpkg + AVAM_P_DATAOFFSET + 4);
			UNPACK32(val[2], g_ackpkg + AVAM_P_DATAOFFSET + 8);
		}

		if (pkg->opt) {
			val[0] = g_freq[pkg->opt - 1][2];
			val[1] = g_freq[pkg->opt - 1][1];
			val[2] = g_freq[pkg->opt - 1][0];

			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET);
			UNPACK32(val[1], g_ackpkg + AVAM_P_DATAOFFSET + 4);
			UNPACK32(val[2], g_ackpkg + AVAM_P_DATAOFFSET + 8);
		}
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_FREQ);
		/* change opt */
		g_ackpkg[3] = pkg->opt;
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_SET_VOLT:
		if (g_toohot)
			break;

		val[0] = (pkg->data[0] << 8) | pkg->data[1];
		debug32("D: V(%x)\n", val[0]);
		if(set_voltage((uint16_t)val[0])) {
			a3222_reset();
			for (i = 0; i < ASIC_COUNT; i++)
				a3222_set_freq(g_freq[i], i);
		}
		break;
	case AVAM_P_SETM:
		memcpy(val, pkg->data, 4);
		val[0] = be32toh(val[0]);
		debug32("D: S(%x)\n", val[0]);
		if (val[0] & 0x80000000)
			a3222_set_spispeed(val[0] & 0x7fffffff);
		break;
	case AVAM_P_TEST:
		memcpy(&val[0], pkg->data, 4);
		val[0] = be32toh(val[0]);
		debug32("D: FULL(%08x)\n", val[0]);
		test_core_count = val[0];
		if (!test_core_count)
			test_core_count = TEST_CORE_COUNT;

		memcpy(&val[0], pkg->data + 4, 4);
		val[0] = be32toh(val[0]);
		debug32("D: V(%x)\n", val[0]);
		set_voltage(val[0]);

		PACK32(pkg->data + 12, &val[2]);
		PACK32(pkg->data + 16, &val[1]);
		PACK32(pkg->data + 20, &val[0]);
		debug32("D: F(%x, %x, %x)\n", val[2], val[1], val[0]);

		a3222_sw_init();
		a3222_reset();
		for (i = 0; i < ASIC_COUNT; i++)
			a3222_set_freq(val, i);

		if (testcores(test_core_count, 1) > 2 * test_core_count)
			led_ctrl(LED_ERR_ON);
		else
			led_ctrl(LED_ERR_OFF);

		set_voltage(ASIC_0V);
		break;
	default:
		break;
	}
}

static void coretest_main()
{
	uint8_t i;
	uint32_t val[3];

	debug32("D: coretest\n");

	set_voltage(ASIC_CORETEST_VOLT);
	a3222_sw_init();
	a3222_reset();
	val[0] = val[1] = val[2] = ASIC_CORETEST_FREQ;
	for (i = 0; i < ASIC_COUNT; i++)
		a3222_set_freq(val, i);

	if (testcores(TEST_CORE_COUNT, 0) > 2 * TEST_CORE_COUNT)
		led_ctrl(LED_ERR_ON);
	else
		led_ctrl(LED_ERR_OFF);

	set_voltage(ASIC_0V);
}

static void prcess_idle(void)
{
	uint8_t i;
	uint32_t val[3];
	/* Power off the AISC */
	set_voltage(ASIC_0V);

	val[0] = val[1] = val[2] = 0;
	for (i = 0; i < ASIC_COUNT; i++) {
		memcpy(g_freq[i], val, sizeof(uint32_t) * 3);
		a3222_set_freq(val, i);
	}

	a3222_sw_init();
	UCOM_Flush();
}

int main(void)
{
	uint16_t adc_val;
	uint8_t cur_state = STATE_NORMAL;

	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	a3222_hw_init();
	a3222_sw_init();
	shifter_init();
	timer_init();
	led_init();
	uart_init();
	adc_init();
	wdt_init(WDT_IDLE);

	debug32("Ver:%s\n", AVAM_VERSION);
	led_ctrl(LED_OFF_ALL);

	coretest_main();
	wdt_enable();
	timer_set(TIMER_ID1, IDLE_TIME);
	set_voltage(ASIC_0V);

	while (42) {
		if (dfu_sig()) {
			dfu_proc();
			usb_reconnect();
		}

		wdt_feed();
		switch (cur_state) {
			case STATE_NORMAL:
				while (UCOM_Read_Cnt()) {
					memset(g_reqpkg, 0, AVAM_P_COUNT);
					UCOM_Read(g_reqpkg);
					process_mm_pkg((struct avalon_pkg*)g_reqpkg);
				}

				if (timer_istimeout(TIMER_ID1)) {
					prcess_idle();
					led_ctrl(LED_IDLE);
					led_ctrl(LED_PG_OFF);
					cur_state = STATE_IDLE;
					break;
				}

				adc_read(ADC_CHANNEL_COPPER, &adc_val);
				if (g_toohot || (adc_val <= ADC_CUT)) {
					if (!g_toohot) {
						prcess_idle();
						led_ctrl(LED_HOT_ALERT);
						g_toohot = true;
					}

					if (adc_val >= ADC_RESUME) {
						g_toohot = false;
						led_ctrl(LED_PG_OFF);
					}
					break;
				}

				led_ctrl(LED_BUSY);
				if (read_power_good())
					led_ctrl(LED_PG_ON);
				else
					led_ctrl(LED_PG_OFF);

				a3222_process();
				break;
			case STATE_IDLE:
				if (UCOM_Read_Cnt())
					cur_state = STATE_NORMAL;
				__WFI();
				break;
		}
	}
}
