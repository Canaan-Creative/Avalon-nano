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
#include "defines.h"
#include "protocol.h"
#include "avalon_a3222.h"
#include "avalon_usb.h"
#include "avalon_wdt.h"

#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

static uint8_t g_a3222_pkg[AVAM_P_WORKLEN];
static uint8_t g_reqpkg[AVAM_P_COUNT];
static uint8_t g_ackpkg[AVAM_P_COUNT];

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

	expected_crc = (pkg->crc[1] & 0xff) | ((pkg->crc[0] & 0xff) << 8);
	actual_crc = crc16(pkg->data, AVAM_P_DATA_LEN);

	if (expected_crc != actual_crc)
		return;

	switch (pkg->type) {
	case AVAM_P_DETECT:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		memcpy(g_ackpkg + AVAM_P_DATAOFFSET, AVAM_VERSION, AVAM_MM_VER_LEN);
		init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_ACKDETECT);
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_WORK:
		/*
		 * idx-1: midstate(32)
		 * idx-2: job_id(1)+ntime(1)+pool_no(2)+nonce2(4) + reserved(14) + data(12)
		 */

		if (pkg->idx != 1 && pkg->idx != 2 && pkg->cnt != 2)
			break;

		memcpy(g_a3222_pkg + ((pkg->idx - 1) * 32), pkg->data, 32);
		if (pkg->idx == 2 && pkg->cnt == 2) {
			if (!g_a3222_pkg[40])
				a3222_push_work(g_a3222_pkg);
			else {
				memcpy(roll_pkg, g_a3222_pkg, AVAM_P_WORKLEN);
				for (i = 0; i < g_a3222_pkg[32]; i++) {
					a3222_roll_work(roll_pkg, 1);
					a3222_push_work(roll_pkg);
				}
			}
		}
		break;
	case AVAM_P_POLLING:
		memset(g_ackpkg, 0, AVAM_P_COUNT);
		if (a3222_get_report_count()) {
			/* P_NONCE: job_id(1)+ntime(1)+pool_no(2)+nonce2(4)+nonce(4) */
			a3222_get_report(g_ackpkg + AVAM_P_DATAOFFSET);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_NONCE);
		} else {
			/* P_STATUS: */
			memcpy(g_ackpkg + AVAM_P_DATAOFFSET, AVAM_VERSION, AVAM_MM_VER_LEN);
			init_mm_pkg((struct avalon_pkg *)g_ackpkg, AVAM_P_STATUS);
		}
		UCOM_Write(g_ackpkg);
		break;
	case AVAM_P_SET_FREQ:
		UNPACK32(val[2], pkg->data);
		UNPACK32(val[1], pkg->data + 4);
		UNPACK32(val[0], pkg->data + 8);

		for (i = 0; i < ASIC_COUNT; i++)
			a3222_set_freq(val, i);
		break;
	default:
		break;
	}
}

int main(void)
{
	Board_Init();
	SystemCoreClockUpdate();

	usb_init();
	a3222_init();

	wdt_init(5);	/* 5 seconds */
	wdt_enable();

	while (42) {
		wdt_feed();

		if (UCOM_Read_Cnt()) {
			memset(g_reqpkg, 0, AVAM_P_COUNT);

			UCOM_Read(g_reqpkg);
			process_mm_pkg((struct avalon_pkg*)g_reqpkg);
		}

		/* Power off the AISC */

		a3222_process();

		/* Sleep until next IRQ happens */
		//__WFI(); /* FIXME: */
	}
}
