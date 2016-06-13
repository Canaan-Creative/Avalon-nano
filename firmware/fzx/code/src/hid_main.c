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

#include "crc.h"
#include "sha2.h"
#include "defines.h"
#include "libfunctions.h"
#include "protocol.h"
#include "avalon_a3218p.h"
#include "avalon_usb.h"
#include "avalon_wdt.h"
#include "avalon_timer.h"
#include "avalon_adc.h"
#include "avalon_uart.h"

#ifdef DEBUG
#include "avalon_test.h"
#endif

//#define DEBUG_VERBOSE

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#define STATE_NORMAL	0
#define STATE_IDLE	1

static uint8_t g_a3218p_pkg[AVAM_P_WORKLEN];
static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];
static uint32_t g_freq[ASIC_COUNT][7];
static uint8_t g_state = STATE_NORMAL;

static uint8_t g_rxbuf[100];
static uint8_t g_rxlen = 0;
static uint8_t g_rxcnt = 0;

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

static unsigned int testcores(uint32_t core_num, uint32_t ret)
{
	uint32_t result[ASIC_COUNT];
	uint8_t  txdat[20];
	uint32_t all = ASIC_COUNT * core_num;
	uint32_t pass_cal_num = 0, pass[ASIC_COUNT], i, j, f = 0;
	uint8_t  golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
	uint8_t	report[A3218P_REPORT_SIZE];

	pass_cal_num = 0;
	memset(result, 0, ASIC_COUNT * sizeof(uint32_t));

	for (i = 0; i < (core_num + 2); i++) {
		wdt_feed();
#ifdef DEBUG_VERBOSE
		debug32("D: core %d\r\n", i);
#endif
		for (j = 0; j < ASIC_COUNT; j++) {
			pass[j] = 0;
			a3218p_push_work(golden_ob);
		}

		a3218p_process();
		delay(70);
		if (i >= 2) {
			while (a3218p_get_report_count()) {
				a3218p_get_report(report);
#ifdef DEBUG_VERBOSE
				debug32("Count = %d \r\n",++f);
				debug32("Report Data: \r\n");
			//	hexdump(report, A3218P_WORK_SIZE);
				hexdump(report, 32);
#endif
				if (0x0001c7a2 == (report[8] << 24 |
								report[9] << 16 |
								report[10] << 8 |
								report[11])) {
					pass[report[6]] = 1;
				} else {
					debug32("D: N = %08x, C = %d\r\n", report[8] << 24 |
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
		debug32("%d \r\n", result[i]);

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

	debug32("E/A: %d/%d\r\n", all - pass_cal_num, all);
	return all - pass_cal_num;
}

static void process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	uint8_t i;
	uint32_t val[3];

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc) {
		debug32("E: crc err (%x-%x)\n", expected_crc, actual_crc);
		return;
	}

	timer_set(TIMER_ID1, IDLE_TIME);
	switch (pkg->type) {
	case AVAM_P_DETECT:
		a3218p_sw_init();
		UCOM_Flush();

		memset(g_ackpkg, 0, AVAM_P_COUNT);
		memset(g_ackpkg + AVAM_P_DATAOFFSET, 0xff, AVAM_MM_DNA_LEN);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN, AVAM_VERSION, AVAM_MM_VER_LEN);
		UNPACK32(ASIC_COUNT, g_ackpkg + AVAM_P_DATAOFFSET + AVAM_MM_DNA_LEN + AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_WORK:
		/*
		 * idx-1: midstate(32)
		 * idx-2: id(6) + reserved(2) + ntime(1) + fan(3) + led(4) + reserved(4) + data(12)
		 */
		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2)
			break;

		memcpy(g_a3218p_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2)
			a3218p_push_work(g_a3218p_pkg);
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0xff, AVAM_P_COUNT);
		if (a3218p_get_report_count()) {
			/* P_NONCE: id(6) + chip_id(1) + ntime(1) + nonce(4) + reserved(1) + usb rb(1) + work rb(1) + nonce rb(1) */
			val[0] = a3218p_get_works_count();
			a3218p_get_report(g_ackpkg + AVAM_P_DATAOFFSET);
			g_ackpkg[AVAM_P_DATAOFFSET + 13] = UCOM_Read_Cnt();
			g_ackpkg[AVAM_P_DATAOFFSET + 14] = (uint8_t)val[0];
			g_ackpkg[AVAM_P_DATAOFFSET + 15] = a3218p_get_report_count();
			if (a3218p_get_report_count()) {
				val[0] = a3218p_get_works_count();
				a3218p_get_report(g_ackpkg + AVAM_P_DATAOFFSET + 16);
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 13] = UCOM_Read_Cnt();
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 14] = (uint8_t)val[0];
				g_ackpkg[AVAM_P_DATAOFFSET + 16 + 15] = a3218p_get_report_count();
			}

			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_NONCE_M);
		} else {
			/* P_STATUS_M: spi speed(4) + led(4) + fan(4) + voltage(4) + frequency(12) + power good(4) */
			val[0] = a3218p_get_spispeed();
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET);
			val[0] = (uint32_t)-1;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 8);
			val[0] = 0x9f;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 12);
			val[0] = 1;
			UNPACK32(val[0], g_ackpkg + AVAM_P_DATAOFFSET + 28);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS_M);
		}
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_SET_FREQ:
		PACK32(pkg->data, &val[2]);
		PACK32(pkg->data + 4, &val[1]);
		PACK32(pkg->data + 8, &val[0]);

		debug32("D: (%d)-F(%x, %x, %x)\n", pkg->opt, val[2], val[1], val[0]);
		if (!pkg->opt) {
			for (i = 0; i < ASIC_COUNT; i++) {
				memcpy(g_freq[i], val, sizeof(uint32_t) * 3);
				a3218p_set_freq(val, i);
			}
		}

		if (pkg->opt) {
			memcpy(g_freq[pkg->opt - 1], val, sizeof(uint32_t) * 3);
			a3218p_set_freq(val, pkg->opt - 1);
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
		val[0] = (pkg->data[0] << 8) | pkg->data[1];
		set_voltage(val[0]);
		debug32("D: V(%x)\r\n", val[0]);
		a3218p_reset();
		for (i = 0; i < ASIC_COUNT; i++)
		    a3218p_set_freq(g_freq[i], i);
		break;
	case AVAM_P_SETM:
		memcpy(val, pkg->data, 4);
		val[0] = be32toh(val[0]);
		debug32("D: S(%x)\n", val[0]);
		if (val[0] & 0x80000000)
			a3218p_set_spispeed(val[0] & 0x7fffffff);
		break;
	default:
		break;
	}
}

static void coretest_main()
{
	uint8_t i;
	uint32_t val[ASIC_COUNT];

	debug32("CoreTest Begin: \r\n");

	set_voltage(ASIC_CORETEST_VOLT);

	a3218p_sw_init();
	a3218p_reset();

	for (i = 0; i < ASIC_COUNT; i++)
		val[i] = ASIC_CORETEST_FREQ;

	for (i = 0; i < ASIC_COUNT; i++)
		a3218p_set_freq(val, i);

	if (testcores(TEST_CORE_COUNT, 0) > TEST_CORE_COUNT)
		debug32("CoreTest Error \r\n\r\n");
	else
		debug32("CoreTest OK! \r\n\r\n");

	set_voltage(ASIC_0V);
}


static void test_gate_core()
{
	uint8_t i;
	uint32_t val[ASIC_COUNT];

	debug32("CoreTest Begin: \r\n");

//	set_voltage(ASIC_CORETEST_VOLT);

	a3218p_sw_init();
	a3218p_reset();

	for (i = 0; i < ASIC_COUNT; i++)
		val[i] = 0;

	for (i = 0; i < ASIC_COUNT; i++)
		a3218p_set_freq(val, i);

	if (testcores(TEST_CORE_COUNT, 0) > TEST_CORE_COUNT)
		debug32("CoreTest Error \r\n\r\n");
	else
		debug32("CoreTest OK! \r\n\r\n");

	//set_voltage(ASIC_0V);
}

static void test_function(void)
{
#ifdef DEBUG

//#define TEST_MAIN_CLOCK
#ifdef TEST_MAIN_CLOCK
	/* 0 clock source is internal oscillator 
	 * 1 clock source is crystal oscillator
	 * 2 clock source is watchdog oscillator
	 * 3 clock source is main clock
	 */
	test_system_clock_out(3);
#endif

//#define TEST_UART
#ifdef TEST_UART
	uart_init();
	delay(1500);
	debug32("Hello World!\r\n");
	delay(1500);
	debug32("Hello Avalon Card!\r\n");
#endif

//#define TEST_LED
#ifdef TEST_LED
	uint8_t i;

	led_init();
	while (1) {
		for (i = 0; i < 8; i++) {
			led_set((1 << i));
			delay(3000);
		}
	}
#endif

//#define TEST_ADC
#ifdef TEST_ADC
#define VCC 3.45
	/* 10Bit ADC
	 * VCC Reference voltage
	 * */
	uint16_t adc_raw_v12,adc_raw_vcore;
	float    adc_v12,adc_vcore;
	
	adc_init();
	while (1) {
		adc_read(ADC_CHANNEL_V12V, &adc_raw_v12);
		adc_read(ADC_CHANNEL_VCORE, &adc_raw_vcore);
		adc_v12   = (float)adc_raw_v12   /1024 * VCC;
		adc_vcore = (float)adc_raw_vcore /1024 * VCC;
		debug32("V12_Raw = %d ,Vore_Raw = %d\r\n", adc_raw_v12, adc_raw_vcore);
		debug32("V12     = %.3f ,Vore   = %.3f\r\n", adc_v12, adc_vcore);
		delay(2000);
	}
#endif

//#define TEST_FAN
#ifdef TEST_FAN
	uint8_t i;
	
	fan_init();
	while (1) {
		for (i = 0; i < 255; i++) {
			fan_adjust_pwm(i);
			delay(100);
		}
	}
#endif

//#define TEST_IIC
#ifdef TEST_IIC
	int temp;

	timer_init();
	i2c_init();
	while (1) {
		temp = i2c_readtemp();
		debug32("Temp = %d\r\n", temp);
		delay(1300);
	}
#endif

//#define TEST_VCORE
#ifdef TEST_VCORE
#define VCC 3.45
	/* 10Bit ADC
	 * VCC Reference voltage
	 * */
	uint16_t i;
	uint16_t adc_raw_vcore;
	float    adc_vcore;

	adc_init();
	vcore_init();
	while (1) {
		for (i = 0; i < 32; i++) {
			set_voltage(i);
			delay(5000);
			adc_read(ADC_CHANNEL_VCORE, &adc_raw_vcore);
			adc_vcore = (float)adc_raw_vcore /1024 * VCC * 11 + 0.0005; 
			debug32("vcore_raw = %d, vcore_value = %0.3f\r\n", i, adc_vcore);
		}
		delay(5000);
		debug32("\r\n\r\n\r\n\r\n");
	}
#endif

//#define TEST_LOOPBACK
#ifdef TEST_LOOPBACK
	uint32_t test_total_asic = 0;

	a3218p_hw_init();
	a3218p_sw_init();

	while (1) {
		test_loopback(&test_total_asic);
		//test_gate_core();
		delay(10);
	}
#endif
#endif
}

int main(void)
{
	Board_Init();
	SystemCoreClockUpdate();

	uart_init();
	test_io_init();

	while (!uart_read(g_rxbuf));
	while (!uart_read(g_rxbuf));
	while (!uart_read(g_rxbuf));

	while (1) { 
		if (!uart_read(g_rxbuf)) {
			if (g_rxbuf[0] & 0x01)	
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 1);
			else
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 0);

			if (g_rxbuf[0] & 0x02)	
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 1);
			else
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 0);

			if (g_rxbuf[0] & 0x04)	
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 1);
			else
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 0);

			if (g_rxbuf[0] & 0x08)	
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 1);
			else
				Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 0);
	
			uart_nwrite(g_rxbuf, 1);
		}
	}

	delay(50);
}
