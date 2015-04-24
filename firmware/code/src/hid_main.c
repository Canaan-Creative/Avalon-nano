/*
 * @brief avalon routines
 *
 * @note
 * Copyright(C) canaan creative, 2015
 * All rights reserved.
 *
 */
#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#include "hid_ucom.h"
#include "avalon_api.h"
#ifdef __CODE_RED
#include <NXP/crp.h>
#endif
#include "crc.h"
#include "sha2.h"
#include "protocol.h"

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
static uint8_t g_a3222_pkg[MM_TASK_LEN];
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
	uint8_t report[A3222_REPORT_SIZE];

	expected_crc = (pkg->crc[1] & 0xff)
			| ((pkg->crc[0] & 0xff) << 8);
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
		if (pkg->idx > pkg->cnt)
			break;
		/*
		 * idx-1: midstate(32)
		 * idx-2: job_id(1)+ntime(1)+pool_no(2)+nonce2(4) + reserved(14) + data(12)
		 * */
		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == pkg->cnt) {
			uint32_t work_id[2];

			work_id[0] = (g_a3222_pkg[32] << 24) |
					(g_a3222_pkg[33] << 16) |
					(g_a3222_pkg[34] << 8) |
					g_a3222_pkg[35];
			work_id[1] = (g_a3222_pkg[36] << 24) |
					(g_a3222_pkg[37] << 16) |
					(g_a3222_pkg[38] << 8) |
					g_a3222_pkg[39];
			AVALON_A3222_Process(g_a3222_pkg, work_id);
		}
		break;
	case AVAU_P_POLLING:
		tmp = AVALON_A3222_ReportCnt();
		memset(g_ackpkg, 0, AVAU_P_COUNT);
		if (tmp > 0) {
			/* P_NONCE: job_id(1)+ntime(1)+pool_no(2)+nonce2(4)+nonce(4) */
			AVALON_A3222_GetReport(report);
			memcpy(g_ackpkg + AVAU_P_DATAOFFSET, report, A3222_REPORT_SIZE);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_NONCE);
		} else {
			/*TODO: P_STATUS: temp etc */
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_STATUS);
		}
		UCOM_Write(g_ackpkg, AVAU_P_COUNT);
		break;
	default:
		break;
	}
}

void AVALON_Delay(unsigned int ms)
{
       unsigned int i;
       unsigned int msticks = SystemCoreClock/16000; /* FIXME: 16000 is not accurate */

       while (ms && ms--) {
               for(i = 0; i < msticks; i++)
                       __NOP();
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

	AVALON_USB_Init();
	AVALON_TMR_Init();

	while (1) {
		switch (a3233_stat) {
		case A3222_STAT_WAITMM:
			memset(g_a3222_pkg, 0, MM_TASK_LEN);
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
