/*
 * @brief avalon routines
 *
 * @note
 * Copyright(C) canaan creative, 2015
 * All rights reserved.
 *
 */
#include "avalon_api.h"
#include "sha2.h"

#define WORK_LEN_BYTE     (23 * 4)
#define LPC_SSP           LPC_SSP0
#define SSP_DATA_BITS     SSP_BITS_8
#define SSP_MODE_MASTER   1

static uint8_t g_txbuf[WORK_LEN_BYTE];
static uint8_t g_rxbuf[WORK_LEN_BYTE];
static uint8_t g_a3222_report[A3222_REPORT_NONCE_CNT * A3222_REPORT_SIZE];
static RINGBUFF_T g_a3222_rxrb;

static uint8_t test_data[] =
{
		0x1b,0xed,0x3b,0xa0,

		0xa2,0xcb,0x45,0xc1,

		0xd8,0xf8,0xef,0x67,
		0x12,0x14,0x64,0x95,
		0xc4,0x41,0x92,0xc0,
		0x71,0x45,0xfd,0x6d,
		0x97,0x4b,0xf4,0xbb,
		0x8f,0x41,0x37,0x1d,
		0x65,0xc9,0x0d,0x1e,
		0x9c,0xb1,0x8a,0x17,

		0xfa,0x77,0xfe,0x7d,
		0x13,0xcd,0xfd,0x7b,
		0x00,0x63,0x91,0x07,
		0x62,0xa5,0xf2,0x5c,
		0x06,0xb1,0x68,0xae,

		0x08,0x7e,0x05,0x1a,
		0x89,0x51,0x70,0x50,
		0x4a,0xc1,0xd0,0x01
};

static void gen_test_work(uint8_t *data, uint8_t index)
{
	unsigned int tmp = 0;
	int j;

	memset(data, 0, WORK_LEN_BYTE);
	for (j = 0; j < 18 * 4; j++)
			data[j] = test_data[j];

	data[72] = 0x55;
	data[75] = index;
	data[79] = index;//nonce2

	data[83] = 0x1;//cpm2
	data[87] = 0x1;//cpm1
	data[91] = 0x1;//cpm0
}
/* Initialize buffer */
static void AVALON_A3222_Buffer_Init(void)
{
	uint16_t i;

	RingBuffer_Init(&g_a3222_rxrb, g_a3222_report, 1, A3222_REPORT_NONCE_CNT * A3222_REPORT_SIZE);
	for (i = 0; i < WORK_LEN_BYTE; i++) {
		g_txbuf[i] = 0;
		g_rxbuf[i] = 0;
	}
}

static void AVALON_A3222_Init_PinMux(void)
{
	/* Only SSP0 is supported */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC0));	/* LOAD */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 29, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* SCK0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* MISO0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_FUNC1 | IOCON_MODE_PULLUP));	/* MOSI0 */
}

static void AVALON_A3222_Load_Set(bool On)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, On);
}

static void AVALON_A3222_Sent_Load(void)
{
	Chip_SSP_DATA_SETUP_T xf_setup;

	xf_setup.length = 8;
	xf_setup.tx_data = g_txbuf;
	xf_setup.rx_data = g_rxbuf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;

	AVALON_A3222_Load_Set(1);
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	AVALON_A3222_Load_Set(0);
}

static void AVALON_A3222_SPI_Init()
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

void AVALON_A3222_Process(uint8_t *work, const uint32_t *work_id, uint8_t test)
{
	int i;
	Chip_SSP_DATA_SETUP_T  xf_setup;
	uint32_t pre_a[3], pre_e[3];
	uint8_t report[A3222_REPORT_SIZE];
	uint32_t last_nonce = 0xbeafbeaf, tmp, ret;

	if (test) {
		memcpy(g_txbuf, work, WORK_LEN_BYTE);
	} else {
		sha256_loc(work , pre_a, pre_e);

		memset(g_txbuf, 0, WORK_LEN_BYTE);	/* nonce */

		g_txbuf[0] = 0x5e;
		g_txbuf[1] = 0x20;
		g_txbuf[2] = 0x88;
		g_txbuf[3] = 0xc8;

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
		UNPACK32(work_id[1], g_txbuf + 76)
		memcpy(g_txbuf + 80, "\x0\x0\x0\x1", 4);
		memcpy(g_txbuf + 84, "\x0\x0\x0\x1", 4);
		memcpy(g_txbuf + 88, "\x0\x0\x0\x1", 4);
	}
	xf_setup.length = WORK_LEN_BYTE;
	xf_setup.tx_data = g_txbuf;
	xf_setup.rx_data = g_rxbuf;
	xf_setup.rx_cnt = 0;
	xf_setup.tx_cnt = 0;
	ret = Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	if (ERROR == ret) {
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

	AVALON_A3222_Sent_Load();
	AVALON_Delay(500);
}

uint8_t AVALON_A3222_ReportCnt(void)
{
	return RingBuffer_GetCount(&g_a3222_rxrb) / A3222_REPORT_SIZE;
}

uint8_t AVALON_A3222_GetReport(uint8_t *report)
{
	if (A3222_REPORT_SIZE == RingBuffer_PopMult(&g_a3222_rxrb, report, A3222_REPORT_SIZE))
		return 0;
	return 1;
}

void AVALON_A3222_Init(void)
{
	AVALON_A3222_Init_PinMux();
	AVALON_A3222_Buffer_Init();
	AVALON_A3222_Load_Set(0);
	AVALON_A3222_SPI_Init();
}

#define AVALON_A3222_TEST
#ifdef AVALON_A3222_TEST
static const int hex2bin_tbl[256] = {
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
         -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 };

/* Does the reverse of bin2hex but does not allocate any ram */
static unsigned char hex2bin(unsigned char *p, const unsigned char *hexstr, unsigned    int len)
{
        int nibble1, nibble2;
        unsigned char idx;
        unsigned char ret = 0;

        while (*hexstr && len) {
                if ((!hexstr[1])) {
                        return ret;
                }

                idx = *hexstr++;
                nibble1 = hex2bin_tbl[idx];
                idx = *hexstr++;
                nibble2 = hex2bin_tbl[idx];

                if (((nibble1 < 0) || (nibble2 < 0))) {
                        return ret;
                }

                *p++ = (((unsigned char)nibble1) << 4) | ((unsigned char)nibble2);
                --len;
        }

        if (len == 0 && *hexstr == 0)
                ret = 1;
        return ret;
}

void AVALON_A3222_Test(void)
{
	/*
	 * header:
	 * 00000003b968b96fd5c2e94facad8296e7e8a9a87fa565810108fb7600000000
	 * 00000000cb59ccf140d92666713f71b56cd2702b60383c8e9f9685e8fd6fa7f8
	 * d22dfc5955372b8b181717f00000000000000080000000000000000000000000
	 * 0000000000000000000000000000000000000000000000000000000080020000
	 *
	 * data:
	 * 62462b5ee07afe103e86dd9eef330045a12e7215f5fecb6762ac8ad7b9829f8d <-- 32B
	 * 000000000000000000000000 00000000 00000000 						<-- 20B
	 * f01717188b2b375559fc2dd2 										<-- 12B
	 *
	 * nonce:
	 * 5e2199c8
	 */
#if 0
	unsigned char strbuf[] =
			"d8f8ef6712146495c44192c07145fd6d974bf4bb8f41371d65c90d1e9cb18a17"
			"0000000000000000000000000000000000000000"
			"087e051a895170504ac1d001";

#else
	unsigned char strbuf[] =
			"62462b5ee07afe103e86dd9eef330045a12e7215f5fecb6762ac8ad7b9829f8d"
			"0000000000000000000000000000000000000000"
			"f01717188b2b375559fc2dd2";
#endif

	uint8_t buf[WORK_LEN_BYTE], i;
	uint8_t reportcnt, report[A3222_REPORT_SIZE];
	uint32_t work_id[2], nonce;

	AVALON_A3222_Init();

	memset(buf, 0, WORK_LEN_BYTE);
	hex2bin(buf, strbuf, 64);

	for (i = 0; i < 3; i++) {
		work_id[0] = 0x55223380 | i;
		work_id[1] = i;
#if 0
		gen_test_work(buf, i);
		AVALON_A3222_Process(buf, work_id, 1);
#else
		AVALON_A3222_Process(buf, work_id, 0);
#endif
	}
	reportcnt = AVALON_A3222_ReportCnt();
	DEBUGOUT("Report count: %d\n", reportcnt);
	for (i = 0; i < reportcnt; i++) {
		AVALON_A3222_GetReport(report);
		work_id[0] = (report[0] << 24) | (report[1] << 16) | (report[2] << 8) | report[3];
		work_id[1] = (report[4] << 24) | (report[5] << 16) | (report[6] << 8) | report[7];
		nonce = (report[8] << 24) | (report[9] << 16) | (report[10] << 8) | report[11];
		DEBUGOUT("work_id: %08x-%08x, nonce: %08x\n", work_id[0], work_id[1], nonce - 0x4000);
	}
}
#endif
