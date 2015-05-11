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

#include "board.h"
#include "defines.h"
#include "avalon_a3222.h"
#include "protocol.h"
#include "sha2.h"

#define A3222_WORK_SIZE		(23 * 4)
#define A3222_WORK_CNT		8

#define A3222_REPORT_SIZE	12 /* work_id (8bytes) + nonce (4bytes) */
#define A3222_REPORT_CNT	12

#define PIN_LOAD	19
#define PIN_SCK	15
#define PIN_MISO	22
#define PIN_MOSI	21
#define PIN_FPGARESET	2

static uint8_t g_asic_index;
static uint8_t g_freqflag;
static uint32_t g_freq[ASIC_COUNT][3];

static uint8_t g_a3222_works[A3222_WORK_SIZE * A3222_WORK_CNT];
static uint8_t g_a3222_reports[A3222_REPORT_SIZE * A3222_REPORT_CNT];
static RINGBUFF_T a3222_txrb;
static RINGBUFF_T a3222_rxrb;

static void load_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, PIN_LOAD, IOCON_FUNC0);	/* LOAD */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, PIN_LOAD);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, PIN_SCK, (IOCON_FUNC3 | IOCON_MODE_PULLUP));	/* SCK0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_MISO, (IOCON_FUNC3 | IOCON_MODE_PULLUP | IOCON_DIGMODE_EN));	/* MISO0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_MOSI, (IOCON_FUNC2 | IOCON_MODE_PULLUP));	/* MOSI0 */

}

static void load_set(bool On)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 1, PIN_LOAD, On);
}

static void spi_init(void)
{
	Chip_SSP_Init(LPC_SSP1);

	Chip_SSP_SetFormat(LPC_SSP1, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_MODE0);
	Chip_SSP_SetMaster(LPC_SSP1, 1);
	Chip_SSP_SetBitRate(LPC_SSP1, 1*1000*1000);
	Chip_SSP_Enable(LPC_SSP1);
}

static void clk_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
}

void a3222_reset(void)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_FPGARESET);

	/* high -> low -> high */
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, true);
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, false);
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, true);
	__NOP();
}

void a3222_hw_init(void)
{
	load_init();
	spi_init();
	clk_init();

	load_set(0);

	RingBuffer_Init(&a3222_txrb, g_a3222_works, A3222_WORK_SIZE, A3222_WORK_CNT);
	RingBuffer_Init(&a3222_rxrb, g_a3222_reports, A3222_REPORT_SIZE, A3222_REPORT_CNT);
}

void a3222_sw_init(void)
{
	uint8_t i;

	load_set(0);

	g_freqflag = 0;

	for (i = 0; i < ASIC_COUNT; i++) {
		g_freq[i][0] = A3222_DEFAULT_FREQ;
		g_freq[i][1] = A3222_DEFAULT_FREQ;
		g_freq[i][2] = A3222_DEFAULT_FREQ;
	}

	RingBuffer_Flush(&a3222_txrb);
	RingBuffer_Flush(&a3222_rxrb);
}

void a3222_roll_work(uint8_t *pkg, int ntime_offset)
{
	uint32_t timev;

	PACK32(pkg + 56, &timev);
	timev += ntime_offset;
	UNPACK32(timev, pkg + 56);
}

static int a3222_process_spi(uint8_t *spi_txbuf)
{
	int i;
	Chip_SSP_DATA_SETUP_T  xf_setup;

	uint8_t spi_rxbuf[A3222_WORK_SIZE];
	uint8_t report[A3222_REPORT_SIZE];

	uint32_t tmp, ret;
	uint32_t last_nonce = 0xbeafbeaf;

	xf_setup.length = A3222_WORK_SIZE;
	xf_setup.tx_data = spi_txbuf;
	xf_setup.rx_data = spi_rxbuf;
	xf_setup.rx_cnt = 0;
	xf_setup.tx_cnt = 0;
	ret = Chip_SSP_RWFrames_Blocking(LPC_SSP1, &xf_setup);
	if (ret == ERROR) {
		DEBUGOUT("%s-%d: Chip_SSP_RWFrames_Blocking %d failed!\n", __FUNCTION__, __LINE__, ret);
		return 1;
	}

	/* 8 bytes work_id */
	memcpy(report, spi_rxbuf, 8);
	for (i = 0; i < 8; i++) {
		memcpy(report + 8, spi_rxbuf + 8 + i * 4, 4);
		tmp = report[8] << 24 | report[9] << 16 | report[10] << 8 | report[11];
		if (tmp == 0xbeafbeaf || tmp == last_nonce)
			continue;

		last_nonce = tmp;
		RingBuffer_Insert(&a3222_rxrb, report);
	}

	return 0;
}

int a3222_push_work(uint8_t *pkg)
{
	uint32_t pre_a[3], pre_e[3];
	uint8_t awork[A3222_WORK_SIZE];

	sha256_loc(pkg, pkg + 52, pre_a, pre_e);

	memset(awork, 0, A3222_WORK_SIZE);

	pre_a[2] = __REV(pre_a[2]);
	UNPACK32(pre_a[2], awork + 4);    /* a2 */

	memcpy(awork + 8, pkg, 32);		/* midstate */

	pre_e[0] = __REV(pre_e[0]);
	pre_e[1] = __REV(pre_e[1]);
	pre_e[2] = __REV(pre_e[2]);
	pre_a[0] = __REV(pre_a[0]);
	pre_a[1] = __REV(pre_a[1]);
	UNPACK32(pre_e[0], awork + 40);	/* e0 */
	UNPACK32(pre_e[1], awork + 44);	/* e1 */
	UNPACK32(pre_e[2], awork + 48);	/* e2 */
	UNPACK32(pre_a[0], awork + 52);	/* a0 */
	UNPACK32(pre_a[1], awork + 56);	/* a1 */

	memcpy(awork + 60, pkg + 52, 12); /* data */

	memcpy(awork + 72, pkg + 32, 6);	 /* id + reserved */
	awork[78] = g_asic_index;		/* asic */
	awork[79] = 0;				/* ntime */

	if ((g_freqflag >> g_asic_index) & 1) {
		g_freqflag &= ~(1 << g_asic_index);
		UNPACK32(g_freq[g_asic_index][0], awork + 80);
		UNPACK32(g_freq[g_asic_index][1], awork + 84);
		UNPACK32(g_freq[g_asic_index][2], awork + 88);
	} else {
		memcpy(awork + 80, "\x0\x0\x0\x1", 4);
		memcpy(awork + 84, "\x0\x0\x0\x1", 4);
		memcpy(awork + 88, "\x0\x0\x0\x1", 4);	/* PLL, Voltage: 0.7625 */
	}

	g_asic_index++;
	g_asic_index %= ASIC_COUNT;

	return RingBuffer_Insert(&a3222_txrb, awork);
}

int a3222_process(void)
{
	int i;
	uint8_t awork[A3222_WORK_SIZE];
	uint8_t load = 0xff;

	if (a3222_get_works_count() < ASIC_COUNT)
		return 0;

	for (i = 0; i < ASIC_COUNT; i++) {
		RingBuffer_Pop(&a3222_txrb, awork);
		if (a3222_process_spi(awork))
			return 1;
	}

	load_set(1);
	Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &load, 1);	/* A3222 load needs 8 cycle clocks, 1B */
	load_set(0);

	return 0;
}

int a3222_get_works_count(void)
{
	return RingBuffer_GetCount(&a3222_txrb);
}

int a3222_get_report_count(void)
{
	return RingBuffer_GetCount(&a3222_rxrb);
}

int a3222_get_report(uint8_t *report)
{
	return RingBuffer_Pop(&a3222_rxrb, report);
}

void a3222_set_freq(uint32_t *freq, uint8_t index)
{
	if (index >= ASIC_COUNT)
		return;

	if (!memcmp(freq, &g_freq[index], sizeof(uint32_t) * 3))
		return;

	g_freqflag |= (1 << index);
	memcpy(g_freq[index], freq, sizeof(uint32_t) * 3);
}

void a3222_get_freq(uint32_t freq[], uint8_t index)
{
	if (index >= ASIC_COUNT)
		return;

	memcpy(freq, &g_freq[index], sizeof(uint32_t) * 3);
}
