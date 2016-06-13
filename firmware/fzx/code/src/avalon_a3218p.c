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
#include <stdlib.h>

#include "board.h"
#include "defines.h"
#include "avalon_a3218p.h"
#include "protocol.h"
#include "sha2.h"
#include "libfunctions.h"

#define PIN_LOAD	19
#define PIN_SCK		15
#define PIN_MISO	22
#define PIN_MOSI	21

#define PIN_FPGARESET	2
#define DEFALUT_FREQ_SETTIMES	4

//#define DEBUG_VERBOSE	1

static uint8_t g_asic_index;
static uint8_t g_freqflag[ASIC_COUNT];
static uint32_t g_freq[ASIC_COUNT][7];
static uint32_t g_spispeed = 10000;

static uint8_t g_a3218p_works[A3218P_WORK_SIZE * A3218P_WORK_CNT];
static uint8_t g_a3218p_reports[A3218P_REPORT_SIZE * A3218P_REPORT_CNT];
static RINGBUFF_T a3218p_txrb;
static RINGBUFF_T a3218p_rxrb;

#ifdef DEBUG_VERBOSE
static uint8_t spi_rxbuf[A3218P_WORK_SIZE];
#endif
uint8_t printf_buf32[200];

static void load_set(bool On)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 1, PIN_LOAD, On);
}

static int a3218p_process_spi(uint8_t *spi_txbuf)
{
#ifndef DEBUG_VERBOSE
	uint8_t spi_rxbuf[A3218P_WORK_SIZE];
#endif

	Chip_SSP_DATA_SETUP_T  xf_setup;
	uint8_t report[A3218P_REPORT_SIZE];

	uint32_t i, ret;
	uint32_t workid_rx1, workid_rx2;
	uint32_t workid_tx1, workid_tx2;
	uint32_t last_nonce = 0xffffffff;

	PACK32(spi_txbuf + 52,&workid_tx1);
	PACK32(spi_txbuf + 56,&workid_tx2);

	xf_setup.length = A3218P_WORK_SIZE;
	xf_setup.tx_data = spi_txbuf;
	xf_setup.rx_data = spi_rxbuf;
	xf_setup.rx_cnt = 0;
	xf_setup.tx_cnt = 0;
	ret = Chip_SSP_RWFrames_Blocking(LPC_SSP1, &xf_setup);
	if (ret == ERROR) {
		debug32("E: Chip_SSP_RWFrames_Blocking\n");
		return 1;
	}

	/* 8 bytes workid */
	memcpy(report, spi_rxbuf, 8);

	PACK32(report + 0,&workid_rx1);
	PACK32(report + 4,&workid_rx2);

	if (!workid_rx1 && !workid_rx2)
		return 0;

	if (!(((workid_rx1 & 0xff) == 1) || ((workid_rx1 & 0xff) == 4)))
		return 0;

	if (((workid_rx1 >> 16) & 0xff) >= ASIC_COUNT)
		return 0;

	for (i = 0; i < 16; i++) {
		report[7] = spi_rxbuf[8 + i * 5];
		memcpy((report + 8), (spi_rxbuf + 9 + i * 5), 4);
		ret = (report[8] << 24) | (report[9] << 16) | (report[10] << 8) | report[11];
		if ((report[7] == 0xff))
			break;
		if ((ret == last_nonce))
			continue;

		last_nonce = ret;
		RingBuffer_Insert(&a3218p_rxrb, report);
	}
	return 0;
}

static unsigned int sen(unsigned int di, unsigned int si, unsigned int ki)
{
	unsigned int doo, tmp;

	tmp = (si << (31 - 6)) | (si >> 7);
	doo = (di + tmp) ^ ki;
	return doo;
}

static void mx1_en(unsigned int nonce, unsigned int *data_in, unsigned int *midstate_in, unsigned int *magic_out)
{
	int i;
	unsigned int cs;
	unsigned int key[12];
	unsigned int key_tmp[12] = {0xfdbbcc40, 0xb14b69e4, 0x457f5258, 0x09c102f0, 0x578afa21, 0x955b41a2, 0x118d293e, 0x8bd53f17, 0xf4ada35c, 0x35186fe2, 0xc8d52bd9, 0xdf21fb4e};

	for(i = 0; i < 12; i++)
		key[i] = key_tmp[11 - i];

	cs = data_in[2] +
		data_in[1] +
		data_in[0] +
		midstate_in[7] +
		midstate_in[6] +
		midstate_in[5] +
		midstate_in[4] +
		midstate_in[3] +
		midstate_in[2] +
		midstate_in[1] +
		midstate_in[0];

	magic_out[11] = sen(cs, nonce, key[11]);
	magic_out[10] = sen(data_in[2], magic_out[11], key[10]);
	magic_out[ 9] = sen(data_in[1], magic_out[10], key[ 9]);
	magic_out[ 8] = sen(data_in[0], magic_out[ 9], key[ 8]);
	magic_out[ 7] = sen(midstate_in[7], magic_out[ 8], key[ 7]);
	magic_out[ 6] = sen(midstate_in[6], magic_out[ 7], key[ 6]);
	magic_out[ 5] = sen(midstate_in[5], magic_out[ 6], key[ 5]);
	magic_out[ 4] = sen(midstate_in[4], magic_out[ 5], key[ 4]);
	magic_out[ 3] = sen(midstate_in[3], magic_out[ 4], key[ 3]);
	magic_out[ 2] = sen(midstate_in[2], magic_out[ 3], key[ 2]);
	magic_out[ 1] = sen(midstate_in[1], magic_out[ 2], key[ 1]);
	magic_out[ 0] = sen(midstate_in[0], magic_out[ 1], key[ 0]);
	magic_out[12] = nonce;
}

/*
[63:0] reg_check_target;//spi_mosi[183:120];
[7:0]  reg_speed_round ;//spi_mosi[119:112];
[7:0]  reg_speed_less  ;//spi_mosi[111:104];
[7:0]  reg_speed_bingo ;//spi_mosi[103:96];
[31:0] reg_roll_timeout;//spi_mosi[95:64];
       reg_read_only   ;
       reg_fake_en     ;//spi_mosi[35];
       reg_speed_en    ;//spi_mosi[34];
       reg_check_en    ;//spi_mosi[33];
       reg_roll_en     ;//spi_mosi[32];
{reg_check_target, reg_speed_round, reg_speed_less, reg_speed_bingo, reg_roll_timeout, 27'b0, reg_read_only, reg_fake_en, reg_speed_en, reg_check_en, reg_roll_en, 29'b0, `SPI_PKG_CFG};

Example:
uint64_t reg_check_target = 0xffffffffffffffff;
uint8_t  reg_speed_round  = 0;
uint8_t  reg_speed_less   = 0;
uint8_t  reg_speed_bingo  = 0;
uint32_t reg_roll_timeout = 8388608;
uint8_t  reg_read_only    = 0;
uint8_t  reg_fake_en      = 0;
uint8_t  reg_speed_en     = 0;
uint8_t  reg_check_en     = 1;
uint8_t  reg_roll_en      = 1;

a3218p_cfg(reg_check_target, reg_speed_round, reg_speed_less, reg_speed_bingo, reg_roll_timeout, reg_read_only, reg_fake_en, reg_speed_en, reg_check_en, reg_roll_en, *workid);
*/
static void a3218p_cfg(uint64_t reg_check_target, uint8_t reg_speed_round, uint8_t reg_speed_less, uint8_t reg_speed_bingo, uint32_t reg_roll_timeout, uint8_t  reg_read_only, uint8_t reg_fake_en, uint8_t reg_speed_en, uint8_t reg_check_en, uint8_t reg_roll_en)
{
	uint8_t work[A3218P_WORK_SIZE];
	uint8_t load = 0xff;

	memset(work, 0, A3218P_WORK_SIZE);

	work[A3218P_WORK_SIZE- 1] = 2;
	work[A3218P_WORK_SIZE- 2] = 0;
	work[A3218P_WORK_SIZE- 3] = 0;
	work[A3218P_WORK_SIZE- 4] = 0;

	work[A3218P_WORK_SIZE- 5] = reg_read_only << 4 | reg_fake_en << 3 | reg_speed_en << 2 | reg_check_en << 1 | reg_roll_en;
	work[A3218P_WORK_SIZE- 6] = 0;
	work[A3218P_WORK_SIZE- 7] = 0;
	work[A3218P_WORK_SIZE- 8] = 0;

	work[A3218P_WORK_SIZE- 9] = reg_roll_timeout;
	work[A3218P_WORK_SIZE-10] = reg_roll_timeout >> 8;
	work[A3218P_WORK_SIZE-11] = reg_roll_timeout >> 16;
	work[A3218P_WORK_SIZE-12] = reg_roll_timeout >> 24;

	work[A3218P_WORK_SIZE-13] = reg_speed_bingo;
	work[A3218P_WORK_SIZE-14] = reg_speed_less;
	work[A3218P_WORK_SIZE-15] = reg_speed_round;

	work[A3218P_WORK_SIZE-16] = reg_check_target >> 0;
	work[A3218P_WORK_SIZE-17] = reg_check_target >> 8;
	work[A3218P_WORK_SIZE-18] = reg_check_target >> 16;
	work[A3218P_WORK_SIZE-19] = reg_check_target >> 24;
	work[A3218P_WORK_SIZE-20] = reg_check_target >> 32;
	work[A3218P_WORK_SIZE-21] = reg_check_target >> 40;
	work[A3218P_WORK_SIZE-22] = reg_check_target >> 48;
	work[A3218P_WORK_SIZE-23] = reg_check_target >> 56;

	a3218p_process_spi(work);
	a3218p_process_spi(work);

	load_set(1);
	Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &load, 1);	/* A3218p load needs 8 cycle clocks, 1B */
	load_set(0);
}

static uint32_t api_gen_pll(uint32_t pll_freq)
{
	uint32_t my_freq;

	/*0x1 */ uint8_t cfg_en   = 1;
	/*0x1 */ uint8_t gate50   = 0;
	/*0x1 */ uint8_t pll_rst  = 1;
	/*0xf */ uint8_t CLKR     = 0x0;
	/*0x3f*/ uint8_t CLKF     = 0x3f;
	/*0xf */ uint8_t CLKOD    = 0xf;
	/*0x3f*/ uint8_t BWADJ    = 0x3f;
	/*0x1 */ uint8_t INTFB    = 1;
	/*0x1 */ uint8_t BYPASS   = 0;
	/*0x1 */ uint8_t clk_gate = 0;
	/*0x1 */ uint8_t PWRDN    = 0;
	/*0x1 */ uint8_t TEST     = 0;

	switch (pll_freq) {
		case 0: {clk_gate = 1; PWRDN = 1; pll_rst = 0; break;}
		case 50:  {/*CLKR = 0x;*/ CLKF = 13; CLKOD = 0x6; /*BWADJ = 0x;*/ break;}
		case 100: {/*CLKR = 0x;*/ CLKF = 15; CLKOD = 0x3; /*BWADJ = 0x;*/ break;}
		case 125: {/*CLKR = 0x;*/ CLKF = 14; CLKOD = 0x2; /*BWADJ = 0x;*/ break;}
		case 150: {/*CLKR = 0x;*/ CLKF = 17; CLKOD = 0x2; /*BWADJ = 0x;*/ break;}
		case 175: {/*CLKR = 0x;*/ CLKF = 13; CLKOD = 0x1; /*BWADJ = 0x;*/ break;}

		case 200: {/*CLKR = 0x;*/ CLKF = 15; CLKOD = 1; /*BWADJ = 0x;*/ break;}
		case 225: {/*CLKR = 0x;*/ CLKF = 17; CLKOD = 1; /*BWADJ = 0x;*/ break;}
		case 250: {/*CLKR = 0x;*/ CLKF = 19; CLKOD = 1; /*BWADJ = 0x;*/ break;}
		case 275: {/*CLKR = 0x;*/ CLKF = 21; CLKOD = 1; /*BWADJ = 0x;*/ break;}

		case 300: {/*CLKR = 0x;*/ CLKF = 23; CLKOD = 0x1; /*BWADJ = 0x;*/ break;}
		case 325: {/*CLKR = 0x;*/ CLKF = 25; CLKOD = 0x1; /*BWADJ = 0x;*/ break;}
		case 350: {/*CLKR = 0x;*/ CLKF = 13; CLKOD = 0x0; /*BWADJ = 0x;*/ break;}
		case 375: {/*CLKR = 0x;*/ CLKF = 14; CLKOD = 0x0; /*BWADJ = 0x;*/ break;}

		case 400: {/*CLKR = 0x;*/ CLKF = 15; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 425: {/*CLKR = 0x;*/ CLKF = 16; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 450: {/*CLKR = 0x;*/ CLKF = 17; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 475: {/*CLKR = 0x;*/ CLKF = 18; CLKOD = 0; /*BWADJ = 0x;*/ break;}

		case 500: {/*CLKR = 0x;*/ CLKF = 19; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 525: {/*CLKR = 0x;*/ CLKF = 20; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 550: {/*CLKR = 0x;*/ CLKF = 21; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 575: {/*CLKR = 0x;*/ CLKF = 22; CLKOD = 0; /*BWADJ = 0x;*/ break;}

		case 600: {/*CLKR = 0x;*/ CLKF = 23; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 625: {/*CLKR = 0x;*/ CLKF = 24; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 650: {/*CLKR = 0x;*/ CLKF = 25; CLKOD = 0; /*BWADJ = 0x;*/ break;}
		case 675: {/*CLKR = 0x;*/ CLKF = 26; CLKOD = 0; /*BWADJ = 0x;*/ break;}

		default : {/*CLKR = 0x;*/ CLKF = 15; CLKOD = 0x3; /*BWADJ = 0x;*/ break;}
	}
	CLKR = 0x0;
	BWADJ = CLKF;

	my_freq =
		0x1                       |
		((cfg_en   & 0x1 ) <<  1) |
		((gate50   & 0x1 ) <<  2) |
		((pll_rst  & 0x1 ) <<  3) |
                ((CLKR     & 0xf ) <<  4) |
                ((CLKF     & 0x3f) <<  8) |
                ((CLKOD    & 0xf ) << 14) |
                ((BWADJ    & 0x3f) << 18) |
                ((INTFB    & 0x1 ) << 24) |
                ((BYPASS   & 0x1 ) << 25) |
                ((clk_gate & 0x1 ) << 26) |
                ((PWRDN    & 0x1 ) << 27) |
                ((TEST     & 0x1 ) << 28);

	return my_freq;
}

/*
org_dat: {0[32bit], midstate[256bit], data[96bit]}, totally 12word
work len: 23word
*/
static void gen_a3218p_work(unsigned int *org_dat, unsigned int *work, unsigned int ininonce_offset, uint8_t *workid)
{
	unsigned int loc_dat[12];
	unsigned int magic_out[13];
	unsigned int asci_index, j;

	asci_index = (workid[4] << 8) + workid[5];
	if (asci_index >= ASIC_COUNT)
		asci_index = 0;

	for(j = 0; j < 12; j++)
		loc_dat[j] = org_dat[11 - j];


	mx1_en((loc_dat[11] - ininonce_offset), &loc_dat[0], &loc_dat[3], magic_out);

	for (j = 0; j < A3218P_WORK_LEN; j++) {
		if (j == (A3218P_WORK_LEN - 1))
			work[j] = A3218P_CMD_WORK;
		else if (j == 13)
			work[j] = workid[6];
			//work[j] = workid[0] | (workid[1] << 8) | (workid[2] << 16) | (workid[3] << 24);
		else if (j == 14)
			work[j] = 0x12345678;
			//work[j] = workid[4] | (workid[5] << 8) | (workid[6] << 16) | (workid[7] << 24);
			//work[j] = workid[7] | (workid[6] << 8) | (workid[5] << 16) | (workid[4] << 24);
		else if (j > 14)
			work[j] = 0;
		else
			work[j] = magic_out[12 - j];
	}

	if (g_freqflag[g_asic_index]) {
		g_freqflag[g_asic_index] = 0;
		work[A3218P_WORK_LEN - 1] = ((0x8 | DEFAULT_PLLSEL) << 28) | A3218P_CMD_WORK;
		work[A3218P_WORK_LEN - 2] = api_gen_pll(g_freq[g_asic_index][0]);
		work[A3218P_WORK_LEN - 3] = api_gen_pll(g_freq[g_asic_index][1]);
		work[A3218P_WORK_LEN - 4] = api_gen_pll(g_freq[g_asic_index][2]);
		work[A3218P_WORK_LEN - 5] = api_gen_pll(g_freq[g_asic_index][3]);
		work[A3218P_WORK_LEN - 6] = api_gen_pll(g_freq[g_asic_index][4]);
		work[A3218P_WORK_LEN - 7] = api_gen_pll(g_freq[g_asic_index][5]);
		work[A3218P_WORK_LEN - 8] = api_gen_pll(g_freq[g_asic_index][6]);
	}

	if (g_asic_index > 0)
		g_asic_index--;
	else
		g_asic_index = ASIC_COUNT - 1;

	for (j = 0; j < A3218P_WORK_LEN; j++)
		work[j] = bswap_32(work[j]);
}

static void load_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, PIN_LOAD, IOCON_FUNC0);	/* LOAD */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, PIN_LOAD);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, PIN_SCK, (IOCON_FUNC3 | IOCON_MODE_PULLUP));	/* SCK0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_MISO, (IOCON_FUNC3 | IOCON_MODE_PULLUP | IOCON_DIGMODE_EN));	/* MISO0 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_MOSI, (IOCON_FUNC2 | IOCON_MODE_PULLUP));	/* MOSI0 */

}

static void spi_init(void)
{
	Chip_SSP_Init(LPC_SSP1);

	Chip_SSP_SetFormat(LPC_SSP1, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_MODE0);
	Chip_SSP_SetMaster(LPC_SSP1, 1);
	Chip_SSP_SetBitRate(LPC_SSP1, g_spispeed);
	Chip_SSP_Enable(LPC_SSP1);
}

static void clk_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
}

void a3218p_reset(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_FPGARESET, (IOCON_FUNC0 | IOCON_DIGMODE_EN));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_FPGARESET);

	/* high -> low -> high */
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, true);
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, false);
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_FPGARESET, true);
	__NOP();
}

void a3218p_hw_init(void)
{
	load_init();
	spi_init();
	clk_init();

	load_set(0);

	RingBuffer_Init(&a3218p_txrb, g_a3218p_works, A3218P_WORK_SIZE, A3218P_WORK_CNT);
	RingBuffer_Init(&a3218p_rxrb, g_a3218p_reports, A3218P_REPORT_SIZE, A3218P_REPORT_CNT);

	/*
	   reg_check_target = 0xffffffffffffffff;
	   reg_speed_round  = 0;
	   reg_speed_less   = 0;
	   reg_speed_bingo  = 0;
	   reg_roll_timeout = 8388608;
	   reg_read_only    = 0;
	   reg_fake_en      = 0;
	   reg_speed_en     = 0;
	   reg_check_en     = 1;
	   reg_roll_en      = 1;
	*/

	a3218p_cfg(0xffffffffffffffff,
		0,
		0,
		0,
		8388608,
		0,
		0,
		0,
		1,
		1);
}

void a3218p_sw_init(void)
{
	uint8_t i;

	load_set(0);

	g_asic_index = (ASIC_COUNT - 1);

	for (i = 0; i < ASIC_COUNT; i++) {
		g_freq[i][0] = 0;
		g_freq[i][1] = 0;
		g_freq[i][2] = 0;
		g_freq[i][3] = 0;
		g_freq[i][4] = 0;
		g_freq[i][5] = 0;
		g_freq[i][6] = 0;
		g_freqflag[i] = 0;
	}

	RingBuffer_Flush(&a3218p_txrb);
	RingBuffer_Flush(&a3218p_rxrb);
}

int a3218p_push_work(uint8_t *pkg)
{
	uint8_t  awork[A3218P_WORK_SIZE];
	uint32_t org_dat[12];
	uint8_t *p_org_dat = (uint8_t *)org_dat;
	int i;
	uint8_t workid[8];

	p_org_dat[0] = 0;
	p_org_dat[1] = 0;
	p_org_dat[2] = 0;
	p_org_dat[3] = 0;
	for (i = 0; i < 32; i++)
		p_org_dat[i + 4] = pkg[i];

	for (i = 52; i < 64; i++)
		p_org_dat[i + 4 + 256 / 8 - 52] = pkg[i];

	for (i = 0; i < 12; i++)
	{
		org_dat[i] = bswap_32(org_dat[i]);
	}

	memset(awork, 0, A3218P_WORK_SIZE);

	memcpy(workid, pkg + 32, 6);	/* id + reserved */
	workid[6] = g_asic_index;	/* asic */
	workid[7] = 0;			/* ntime */

	gen_a3218p_work(org_dat, (unsigned int *)awork, 0, workid);

#ifdef DEBUG_VERBOSE
	if (RingBuffer_GetCount(&a3218p_txrb) == A3218P_WORK_CNT)
		debug32("E: a3218p_push_work overflow \n");
#endif

	return RingBuffer_Insert(&a3218p_txrb, awork);
}

static void a3218p_gen_bypass_work(uint8_t *work, uint8_t *nonce2, uint8_t *nonce, uint8_t core_id, uint8_t data)
{
	int i;
	uint32_t tmp;
	for (i = 0; i < A3218P_WORK_SIZE; i++)
		work[i] = data + i;

	work[A3218P_WORK_SIZE - 1] = 4;//bypass work
	work[3] = core_id;
	work[2] = 0;
	work[1] = 0;
	work[0] = 0;
	nonce2[0] = work[A3218P_WORK_SIZE - 12];
	nonce2[1] = work[A3218P_WORK_SIZE - 11];
	nonce2[2] = work[A3218P_WORK_SIZE - 10];
	nonce2[3] = work[A3218P_WORK_SIZE -  9];
	nonce2[4] = work[A3218P_WORK_SIZE -  8];
	nonce2[5] = work[A3218P_WORK_SIZE -  7];
	nonce2[6] = work[A3218P_WORK_SIZE -  6];
	nonce2[7] = work[A3218P_WORK_SIZE -  5];

	nonce[0] = core_id<<1;
	tmp = (core_id << 16) | (core_id << 8) | (core_id);
	for (i = 0; i < 76; i++) {
		tmp = tmp + work[4+i];
	}
	nonce[1] = tmp >> 16;
	nonce[2] = tmp >>  8;
	nonce[3] = tmp >>  0;
}

static int send_a3218p_spi(uint8_t *spi_txbuf, uint8_t *spi_rxbuf, uint8_t spi_cnt)
{
	Chip_SSP_DATA_SETUP_T  xf_setup;
	uint8_t load = 0xff;
	uint32_t ret;
	uint8_t i;

	for (i = 0; i < spi_cnt; i++) {
		xf_setup.length = A3218P_WORK_SIZE;
		xf_setup.tx_data = spi_txbuf;
		xf_setup.rx_data = spi_rxbuf;
		xf_setup.rx_cnt = 0;
		xf_setup.tx_cnt = 0;
		ret = Chip_SSP_RWFrames_Blocking(LPC_SSP1, &xf_setup);
		if (ret == ERROR) {
			debug32("E: Chip_SSP_RWFrames_Blocking\n");
			return 1;
		}
	}

	load_set(1);
	Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &load, 1);	/* A3222 load needs 8 cycle clocks, 1B */
	load_set(0);

	return 0;
}

void a3218p_bypass_test(void)
{
	uint8_t work[A3218P_WORK_SIZE];
	uint8_t spi_rxbuf[A3218P_WORK_SIZE];
	uint8_t nonce2[8];
	uint8_t nonce[4];
	uint8_t core_id;
	int i, j, k;

	a3218p_cfg(0xffffffffffffffff,
		0,
		0,
		0,
		8388608,
		0,
		0,
		0,
		0,
		0);

	memset(work, 0, sizeof(uint8_t) * A3218P_WORK_SIZE);
	debug32("\r\n");
	for (k = 0; k < 1000; k++) {
		debug32("\r\n\r\n[Round = %d]\r\n", k);
		for (i = 0; i < 8*8+2; i++) {
			core_id = i;
			a3218p_gen_bypass_work(work, nonce2, nonce, core_id, core_id+k);
			debug32("[MOSI(%03d) = ]", i);
			for (j = 0; j < A3218P_WORK_SIZE; j++)
				debug32("%02x", work[j]);
			debug32("\r\n");
			//delay(3000);
			send_a3218p_spi(work, spi_rxbuf,2);

			debug32("[MISO(%03d) = ]", i);
			for (j = 0; j < A3218P_WORK_SIZE; j++)
				debug32("%02x", spi_rxbuf[j]);
			if (i >= 5) {
				a3218p_gen_bypass_work(work, nonce2, nonce, core_id-2, core_id-2+k);
				if (nonce2[0] == spi_rxbuf[0] && nonce2[1] == spi_rxbuf[1] && nonce2[2] == spi_rxbuf[2] && nonce2[3] == spi_rxbuf[3] &&
						nonce2[4] == spi_rxbuf[4] && nonce2[5] == spi_rxbuf[5] && nonce2[6] == spi_rxbuf[6] && nonce2[7] == spi_rxbuf[7] &&
						spi_rxbuf[8] == 0
						&& nonce[0] == spi_rxbuf[9] && nonce[1] == spi_rxbuf[10] && nonce[2] == spi_rxbuf[11] && nonce[3] == spi_rxbuf[12]
				  ) {
					debug32("\r\n[PASS!]");
				} else {
					debug32("\r\nget_nonce2 = %x %x %x %x %x %x %x %x", spi_rxbuf[0], spi_rxbuf[1], spi_rxbuf[2], spi_rxbuf[3], spi_rxbuf[4], spi_rxbuf[5], spi_rxbuf[6], spi_rxbuf[7]);
					debug32("\r\nexp_nonce2 = %x %x %x %x %x %x %x %x", nonce2[0], nonce2[1], nonce2[2], nonce2[3], nonce2[4], nonce2[5], nonce2[6], nonce2[7]);
					debug32("\r\n[FAIL!]");
					while(1);
				}
			}
			debug32("\r\n");
			debug32("\r\n");
		}
	}
}

int a3218p_process(void)
{
	int i,j;
	uint8_t awork[A3218P_WORK_SIZE];
	uint8_t load = 0xff;

	if (a3218p_get_works_count() < ASIC_COUNT)
		return 0;

	for (i = 0; i < ASIC_COUNT; i++) {
		RingBuffer_Pop(&a3218p_txrb, awork);
		if (a3218p_process_spi(awork))
			return 1;
#ifdef  DEBUG_VERBOSE
		debug32("TX: \r\n");
			hexdump(awork, A3218P_WORK_SIZE);
		debug32("RX: \r\n");
			hexdump(spi_rxbuf, A3218P_WORK_SIZE);
#endif
	}

	load_set(1);
	Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &load, 1);	/* A3218p load needs 8 cycle clocks, 1B */
	load_set(0);

	return 0;
}

int a3218p_get_works_count(void)
{
	return RingBuffer_GetCount(&a3218p_txrb);
}

int a3218p_get_report_count(void)
{
	return RingBuffer_GetCount(&a3218p_rxrb);
}

int a3218p_get_report(uint8_t *report)
{
	return RingBuffer_Pop(&a3218p_rxrb, report);
}

void a3218p_set_freq(uint32_t *freq, uint8_t index)
{
	if (index >= ASIC_COUNT)
		return;

	g_freqflag[index] = 1;
	memcpy(g_freq[index], freq, sizeof(uint32_t) * 7);
}

void a3218p_get_freq(uint32_t freq[], uint8_t index)
{
	if (index >= ASIC_COUNT)
		return;

//	memcpy(freq, &g_freq[index], sizeof(uint32_t) * 7);
	freq[0] = 0x1e278447;
	freq[1] = 0x1e278447;
	freq[2] = 0x1e278447;
	freq[3] = 0;
	freq[4] = 0;
	freq[5] = 0;
	freq[6] = 0;
}

void a3218p_set_spispeed(uint32_t speed)
{
	if (g_spispeed == speed)
		return;

	g_spispeed = speed;
	Chip_SSP_SetBitRate(LPC_SSP1, speed);
}

uint32_t a3218p_get_spispeed(void)
{
	return g_spispeed;
}

uint32_t test_loopback(uint32_t *asics)
{
	Chip_SSP_DATA_SETUP_T  xf_setup;

	uint8_t  work[A3218P_WORK_SIZE];
	uint8_t  ret[A3218P_WORK_SIZE];
	uint32_t work_index = 0;
	uint32_t total_works= 0;
	uint32_t tmp = 0;
	uint8_t  rett;

	*asics = 0;

	for (work_index = 0; work_index < (ASIC_COUNT * 3 - work_index); work_index++) {
		memset(work, 0, A3218P_WORK_SIZE);
		memset(ret,  0, A3218P_WORK_SIZE);
		UNPACK32((0x41560000) | (ASIC_COUNT * 3 - work_index), (work + 1));
		UNPACK32(A3218P_CMD_LOOPBACK, (work + A3218P_WORK_SIZE - 4));

		xf_setup.length = A3218P_WORK_SIZE;
		xf_setup.tx_data = work;
		xf_setup.rx_data = ret;
		xf_setup.rx_cnt = 0;
		xf_setup.tx_cnt = 0;

		rett = Chip_SSP_RWFrames_Blocking(LPC_SSP1, &xf_setup);
		if (rett == ERROR) {
#ifdef DEBUG
			debug32("E: Chip_SSP_RWFrames_Blocking\n");
#endif
			return 1;
		}

		total_works++;

		if (work_index >= ASIC_COUNT) {
#ifdef DEBUG
			debug32("Rx:\r\n");
			hexdump(ret, A3218P_WORK_SIZE);
#endif
			PACK32(&ret[A3218P_WORK_SIZE - 4], &tmp);
			if (A3218P_CMD_LOOPBACK != tmp)
			continue;

			PACK32(&ret[1], &tmp);

			if (tmp == (0x41560000 | (ASIC_COUNT * 3))) {
				*asics = (total_works - 1) /2;
				break;
			}
		}

	}

	return *asics;
}









