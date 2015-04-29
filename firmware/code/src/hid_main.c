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
#include "protocol.h"
#include "avalon_a3222.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif


static enum current_stat {
	CURRENT_STAT_IDLE = 1,
	CURRENT_STAT_PROCMM
};

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

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAU_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return;

	switch (pkg->type) {
	case AVAU_P_DETECT:
		memset(g_ackpkg, 0, AVAU_P_COUNT);
		memcpy(g_ackpkg + AVAU_P_DATAOFFSET, AVAU_VERSION, AVAU_VERSION_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_ACKDETECT);
		UCOM_Write(g_ackpkg);
		break;
	case AVAU_P_WORK:
		/*
		 * idx-1: midstate(32)
		 * idx-2: job_id(1)+ntime(1)+pool_no(2)+nonce2(4) + reserved(14) + data(12)
		 */

		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2)
			break;

		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			a3222_push_work(g_a3222_pkg);
			a3222_process();
		}
		break;
	case AVAU_P_POLLING:
		memset(g_ackpkg, 0, AVAU_P_COUNT);
		if (a3222_get_report_count()) {
			/* P_NONCE: job_id(1)+ntime(1)+pool_no(2)+nonce2(4)+nonce(4) */
			a3222_get_report(g_ackpkg + AVAU_P_DATAOFFSET);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_NONCE);
		} else {
			/* P_STATUS: */
			g_ackpkg[AVAU_P_DATAOFFSET] = 0xaa;
			g_ackpkg[AVAU_P_DATAOFFSET + 31] = 0x55;
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAU_P_STATUS);
		}
		UCOM_Write(g_ackpkg);
		break;
	default:
		break;
	}
}

int main(void)
{
	enum current_stat stat= CURRENT_STAT_IDLE;

	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	a3222_init();

	wdt_init(5);	/* 5 seconds */
	wdt_enable();

	while (42) {
		wdt_feed();

		if (UCOM_Read_Cnt()) {
			memset(g_reqpkg, 0, AVAU_P_COUNT);

			UCOM_Read(g_reqpkg);
			process_mm_pkg((struct avalon_pkg*)g_reqpkg);
		}

		/* Sleep until next IRQ happens */
		__WFI();
	}
}
