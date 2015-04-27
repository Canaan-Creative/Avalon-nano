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
#include "board.h"
#include "avalon_a3222.h"
#include "protocol.h"
#include "sha2.h"

#define LPC_SSP           LPC_SSP0
#define SSP_DATA_BITS     SSP_BITS_8
#define SSP_MODE_MASTER   1

static uint8_t g_txbuf[A3222_WORK_SIZE];
static uint8_t g_rxbuf[A3222_WORK_SIZE];
static uint8_t g_a3222_report[A3222_REPORT_CNT * A3222_REPORT_SIZE];
static RINGBUFF_T g_a3222_rxrb;

static void Delay(unsigned int ms)
{
       unsigned int i;
       unsigned int msticks = SystemCoreClock / 16000; /* FIXME: 16000 is not accurate */

       while (ms && ms--) {
               for(i = 0; i < msticks; i++)
                       __NOP();
       }
}

/* Initialize buffer */
static void Buffer_Init(void)
{
	RingBuffer_Init(&g_a3222_rxrb, g_a3222_report, 1, A3222_REPORT_CNT * A3222_REPORT_SIZE);
	memset(g_txbuf, 0, A3222_WORK_SIZE);
	memset(g_rxbuf, 0, A3222_WORK_SIZE);
}

static void Init_PinMux(void)
{
	/* Only SSP0 is supported */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC0));	/* LOAD */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 29, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* SCK0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* MISO0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* MOSI0 */
}

static void Load_Set(bool On)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, On);
}

static void Sent_Load(void)
{
	Chip_SSP_DATA_SETUP_T xf_setup;

	xf_setup.length = 8;
	xf_setup.tx_data = g_txbuf;
	xf_setup.rx_data = g_rxbuf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;

	Load_Set(1);
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	Load_Set(0);
}

static void SPI_Init()
{
	SSP_ConfigFormat ssp_format;

	Chip_SSP_Init(LPC_SSP);
	ssp_format.frameFormat = SSP_FRAMEFORMAT_SPI;
	ssp_format.bits = SSP_DATA_BITS;
	ssp_format.clockMode = SSP_CLOCK_MODE0;

	Chip_SSP_SetFormat(LPC_SSP, ssp_format.bits, ssp_format.frameFormat, ssp_format.clockMode);
	Chip_SSP_SetMaster(LPC_SSP, SSP_MODE_MASTER);
	Chip_SSP_Enable(LPC_SSP);
}

static inline uint16_t bswap_16(uint16_t value)
{
        return ((((value) & 0xff) << 8) | ((value) >> 8));
}

static inline uint32_t bswap_32(uint32_t value)
{
	return (((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
        (uint32_t)bswap_16((uint16_t)((value) >> 16)));
}

void Process(uint8_t *work, const uint32_t *work_id)
{
	Chip_SSP_DATA_SETUP_T  xf_setup;

	int i;
	uint8_t report[A3222_REPORT_SIZE];
	uint32_t pre_a[3], pre_e[3];
	uint32_t tmp, ret;
	static uint32_t last_nonce = 0xbeafbeaf;

	sha256_loc(work , pre_a, pre_e);

	memset(g_txbuf, 0, A3222_WORK_SIZE);

	g_txbuf[0] = 0x5e;
	g_txbuf[1] = 0x20;
	g_txbuf[2] = 0x88;
	g_txbuf[3] = 0xc8;					/* nonce */

	pre_a[2] = bswap_32(pre_a[2]);
	UNPACK32(pre_a[2], g_txbuf + 4);    /* a2 */

	memcpy(g_txbuf + 8, work, 32);		/* midstate */

	pre_e[0] = bswap_32(pre_e[0]);
	pre_e[1] = bswap_32(pre_e[1]);
	pre_e[2] = bswap_32(pre_e[2]);
	pre_a[0] = bswap_32(pre_a[0]);
	pre_a[1] = bswap_32(pre_a[1]);
	UNPACK32(pre_e[0], g_txbuf + 40);	/* e0 */
	UNPACK32(pre_e[1], g_txbuf + 44);	/* e1 */
	UNPACK32(pre_e[2], g_txbuf + 48);	/* e2 */
	UNPACK32(pre_a[0], g_txbuf + 52);	/* a0 */
	UNPACK32(pre_a[1], g_txbuf + 56);	/* a1 */

	memcpy(g_txbuf + 60, work + 52, 12); /* data */

	memset(g_txbuf + 72, 0, 8);
	UNPACK32(work_id[0], g_txbuf + 72)
	UNPACK32(work_id[1], g_txbuf + 76)	/* work_id */

	memcpy(g_txbuf + 80, "\x0\x0\x0\x1", 4);
	memcpy(g_txbuf + 84, "\x0\x0\x0\x1", 4);
	memcpy(g_txbuf + 88, "\x0\x0\x0\x1", 4);	/* PLL */

	xf_setup.length = A3222_WORK_SIZE;
	xf_setup.tx_data = g_txbuf;
	xf_setup.rx_data = g_rxbuf;
	xf_setup.rx_cnt = 0;
	xf_setup.tx_cnt = 0;
	ret = Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	if (ret == ERROR) {
		DEBUGOUT("%s-%d: Chip_SSP_RWFrames_Blocking %d failed!\n", __FUNCTION__, __LINE__, ret);
		return;
	}

	/* work_id */
	memcpy(report, g_rxbuf, 8);
	for (i = 0; i < 8; i++) {
		memcpy(report + 8, g_rxbuf + 8 + i * 4, 4);
		tmp = report[8] << 24 | report[9] << 16 | report[10] << 8 | report[11];
		if (tmp == 0xbeafbeaf || tmp == last_nonce)
			continue;

		/* nonce */
		last_nonce = tmp;
		RingBuffer_InsertMult(&g_a3222_rxrb, report, A3222_REPORT_SIZE);
	}

	Sent_Load();
	Delay(500);
}

uint8_t ReportCnt(void)
{
	return RingBuffer_GetCount(&g_a3222_rxrb) / A3222_REPORT_SIZE;
}

uint8_t GetReport(uint8_t *pkg)
{
	uint8_t report[A3222_REPORT_SIZE];

	if (A3222_REPORT_SIZE == RingBuffer_PopMult(&g_a3222_rxrb, report, A3222_REPORT_SIZE)) {
		memcpy(pkg + AVAU_P_DATAOFFSET, report, A3222_REPORT_SIZE);
		return 0;
	}

	return 1;
}
