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

#include "crc.h"
#include "sha2.h"
#include "protocol.h"
#include "avalon_a3222.h"
#include "avalon_timer.h"

#define A3222_TIMER_TIMEOUT				(AVALON_TMR_ID1)
#define A3222_STAT_IDLE					1
#define A3222_STAT_WAITMM				2
#define A3222_STAT_PROCMM				3
#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static uint8_t g_a3222_pkg[AVAU_P_WORKLEN];
static uint8_t g_reqpkg[AVAU_P_COUNT];
static uint8_t g_ackpkg[AVAU_P_COUNT];

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

static void process_mm_pkg(struct avalon_pkg *pkg)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	int ret, tmp;

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAU_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return;

	switch (pkg->type) {
	case AVAU_P_DETECT:
		memset(g_ackpkg, 0, AVAU_P_COUNT);
		memcpy(g_ackpkg + AVAU_P_DATAOFFSET, AVAU_VERSION, AVAU_VERSION_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_ACKDETECT);
		UCOM_Write(g_ackpkg, AVAU_P_COUNT);
		break;
	case AVAU_P_WORK:
		if (pkg->idx != 1 || pkg->idx != 2 || pkg->cnt != 2)
			break;

		if (pkg->idx == 1)
			memset(g_a3222_pkg, 0, AVAU_P_WORKLEN);
		/*
		 * idx-1: midstate(32)
		 * idx-2: job_id(1)+ntime(1)+pool_no(2)+nonce2(4) + reserved(14) + data(12)
		 * */
		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			uint32_t work_id[2];

			PACK32(g_a3222_pkg + 32, &work_id[0]);
			PACK32(g_a3222_pkg + 36, &work_id[1]);

			Process(g_a3222_pkg, work_id);
		}
		break;
	case AVAU_P_POLLING:
		memset(g_ackpkg, 0, AVAU_P_COUNT);
		if (ReportCnt()) {
			/* P_NONCE: job_id(1)+ntime(1)+pool_no(2)+nonce2(4)+nonce(4) */
			GetReport(g_ackpkg);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_NONCE);
		} else {
			/* P_STATUS: temperature etc */
			tmp = UCOM_Read_Cnt();
			g_ackpkg[AVAU_P_DATAOFFSET] = 0xaa;
			g_ackpkg[AVAU_P_DATAOFFSET + 1] = tmp >> 16;
			g_ackpkg[AVAU_P_DATAOFFSET + 2] = tmp >> 8;
			g_ackpkg[AVAU_P_DATAOFFSET + 3] = tmp & 0xff;
			g_ackpkg[AVAU_P_DATAOFFSET + 31] = 0x55;
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_STATUS);
		}
		UCOM_Write(g_ackpkg, AVAU_P_COUNT);
		break;
	default:
		break;
	}
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */

int main(void)
{
	unsigned int buflen = 0;
	Bool timestart = FALSE;
	uint8_t a3233_stat = A3222_STAT_WAITMM;

	Board_Init();
	SystemCoreClockUpdate();

	/* Initialize Avalon chip */
	AVALON_USB_Init();
	AVALON_TMR_Init();

	while (1) {
		switch (a3233_stat) {
		case A3222_STAT_WAITMM:
			buflen = UCOM_Read_Cnt();
			if (buflen >= AVAU_P_COUNT) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3222_TIMER_TIMEOUT);
				a3233_stat = A3222_STAT_PROCMM;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3222_TIMER_TIMEOUT, 50, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3222_TIMER_TIMEOUT)) {
				/* no data */
				timestart = FALSE;
				AVALON_TMR_Kill(A3222_TIMER_TIMEOUT);
				a3233_stat = A3222_STAT_IDLE;
			}
			break;
		case A3222_STAT_IDLE:
			/* TODO: power off the asic */
			buflen = UCOM_Read_Cnt();
			if (buflen >= AVAU_P_COUNT)
				a3233_stat = A3222_STAT_PROCMM;
			break;
		case A3222_STAT_PROCMM:
			if (UCOM_Read_Cnt() >= AVAU_P_COUNT) {
				memset(g_reqpkg, 0, AVAU_P_COUNT);
				UCOM_Read(g_reqpkg, AVAU_P_COUNT);
				process_mm_pkg((struct avalon_pkg*)g_reqpkg);
				timestart = FALSE;
				AVALON_TMR_Kill(A3222_TIMER_TIMEOUT);
			}

			if (!timestart) {
				AVALON_TMR_Set(A3222_TIMER_TIMEOUT, 400, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3222_TIMER_TIMEOUT)) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3222_TIMER_TIMEOUT);
				a3233_stat = A3222_STAT_IDLE;
			}
			break;
		default:
			a3233_stat = A3222_STAT_IDLE;
			break;
		}
		/* Sleep until next IRQ happens */
		__WFI();
	}
}
