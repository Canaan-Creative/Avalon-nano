/*
 * @brief USB to UART bridge example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#include "cdc_uart.h"
#include "avalon_api.h"
#ifdef __CODE_RED
#include <NXP/crp.h>
#endif
#include "sha2.h"
#include "protocol.h"

#define A3233_TASK_LEN 88
#define A3233_NONCE_LEN	4
#define ICA_TASK_LEN 64
#define A3233_TIMER_TIMEOUT				(AVALON_TMR_ID1)
#define A3233_STAT_IDLE					1
#define A3233_STAT_WAITICA				2
#define A3233_STAT_CHKICA				3
#define A3233_STAT_PROCICA				4
#define A3233_STAT_RCVNONCE				5
#define A3233_STAT_PROTECT				6
#define A3233_STAT_MM_PROC				7
#ifdef __CODE_RED
__CRP unsigned int CRP_WORD = CRP_NO_ISP;
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
static unsigned char 	golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
static unsigned int 	a3233_stat = A3233_STAT_WAITICA;
static uint8_t MM_VERSION[MM_VERSION_LEN] = "201408";

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

static int avalon_init_pkg(uint8_t *p,int type)
{

			uint16_t crc;
			p[0] = AVA2_H1;
			p[1] = AVA2_H2;

			p[2] = type;
			p[3] = 1;
			p[4] = 1;
			crc = crc16(p+5, AVA2_P_DATA_LEN);
			p[AVA2_P_COUNT - 2] = crc & 0x00ff;
			p[AVA2_P_COUNT - 1] = (crc & 0xff00) >> 8;
			return 0;
}

static int decode_pkg(uint8_t *p)
{
	unsigned int expected_crc;
	unsigned int actual_crc;
	int idx;
	int cnt;
	int ret;
	uint8_t *data = p + 5;

	idx = p[3];
	cnt = p[4];

	expected_crc = (p[AVA2_P_COUNT - 1] & 0xff) | ((p[AVA2_P_COUNT - 2] & 0xff) << 8);
	actual_crc = crc16(data, AVA2_P_DATA_LEN);
	actual_crc = crc16(data, AVA2_P_DATA_LEN);

	if(expected_crc != actual_crc) {
		return AVA2_P_CRCERROR;
	}

	return p[2];
}

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */

int main(void)
{
	uint8_t 		icarus_buf[ICA_TASK_LEN];
	uint8_t 		mm_buffer[AVA2_P_COUNT];
	unsigned int	icarus_buflen = 0;
	unsigned char 	work_buf[A3233_TASK_LEN];
	unsigned char	nonce_buf[A3233_NONCE_LEN];
	unsigned int	nonce_buflen = 0;
	unsigned int	last_freq = 0;
	uint32_t 		nonce_value = 0;
	Bool			isgoldenob = FALSE;
	Bool			timestart = FALSE;
	int				tmp;
	uint8_t ret_pkg[AVA2_P_COUNT];


	Board_Init();
	SystemCoreClockUpdate();

	/* Initialize avalon chip */
	AVALON_USB_Init();
	AVALON_TMR_Init();
	AVALON_LED_Init();
	AVALON_A3233_Init();

	while (1) {
		switch (a3233_stat) {
		case A3233_STAT_WAITICA:
			icarus_buflen = UCOM_Read_Cnt();
			if (icarus_buflen > 0) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 50, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				/* no data */
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_IDLE;
			}
			break;

		case A3233_STAT_IDLE:
			AVALON_LED_Rgb(AVALON_LED_GREEN);
			icarus_buflen = UCOM_Read_Cnt();
			if (icarus_buflen > 0) {
				a3233_stat = A3233_STAT_CHKICA;
			}

			if (AVALON_A3233_IsPowerEn()) {
				AVALON_A3233_PowerEn(FALSE);
			}
			break;

		case A3233_STAT_CHKICA:
			if (AVALON_A3233_IsTooHot()) {
				a3233_stat = A3233_STAT_PROTECT;
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				break;
			}

			icarus_buflen = UCOM_Read_Cnt();
			if (icarus_buflen >= AVA2_P_COUNT) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_MM_PROC;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 80, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				/* data format error */
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				UCOM_FlushRxRB();
				a3233_stat = A3233_STAT_IDLE;
			}
			break;

		case A3233_STAT_PROTECT:
			if (!AVALON_A3233_IsTooHot()) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
				break;
			}

			if (AVALON_A3233_IsPowerEn())
				AVALON_A3233_PowerEn(FALSE);

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 10000, NULL);
				AVALON_LED_Blink(AVALON_LED_RED);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 10000, NULL);
			}
			break;

		case A3233_STAT_MM_PROC:
			memset(mm_buffer, 0, AVA2_P_COUNT);
			UCOM_Read(mm_buffer, AVA2_P_COUNT);
			tmp = decode_pkg(mm_buffer);
			switch(tmp) {
				case AVA2_P_DETECT:
					memset(ret_pkg,0,AVA2_P_COUNT);
					memcpy(ret_pkg+21,&MM_VERSION,MM_VERSION_LEN);
					avalon_init_pkg(ret_pkg,AVA2_P_ACKDETECT);
					UCOM_Write(ret_pkg,AVA2_P_COUNT);
					a3233_stat = A3233_STAT_WAITICA;
					break;
				case AVA2_P_MIDSTATE:
					memset(icarus_buf,0,ICA_TASK_LEN);
					memcpy(icarus_buf,mm_buffer+5,32);
					memset(ret_pkg,0,AVA2_P_COUNT);
					avalon_init_pkg(ret_pkg,AVA2_P_ACKMIDSTATE);
					a3233_stat = A3233_STAT_WAITICA;
					break;

				case AVA2_P_DATA:
					memcpy(icarus_buf+32,mm_buffer+5,32);
					a3233_stat = A3233_STAT_PROCICA;
					break;

				case AVA2_P_CRCERROR:
					a3233_stat = A3233_STAT_WAITICA;
					break;

				default:
					a3233_stat = A3233_STAT_WAITICA;
				}
			AVALON_LED_Rgb(AVALON_LED_BLUE);
			break;

		case A3233_STAT_PROCICA:

			memset(work_buf, 0, A3233_TASK_LEN);

			if (!memcmp(golden_ob, icarus_buf, ICA_TASK_LEN))
				isgoldenob = TRUE;
			else
				isgoldenob = FALSE;

			data_convert(icarus_buf);
			data_pkg(icarus_buf, work_buf);

			if (!AVALON_A3233_IsPowerEn() || (last_freq != AVALON_A3233_FreqNeeded())) {
				last_freq = AVALON_A3233_FreqNeeded();
				AVALON_A3233_PowerEn(TRUE);
				AVALON_A3233_Reset();
				((unsigned int*)work_buf)[1] = AVALON_A3233_PllCfg(last_freq, NULL);
			}

			{
				unsigned int r,g,b;
				r = last_freq - AVALON_A3233_FreqMin();
				if (r > 255 )
					r = 255;
				g = 0;
				b = 255 - r;
				AVALON_LED_Rgb((r << 16) | (g << 8) | b);
			}

			if (isgoldenob) {
				work_buf[81] = 0x1;
				work_buf[82] = 0x73;
				work_buf[83] = 0xa2;
			}

			UART_Write(work_buf, A3233_TASK_LEN);
			UART_FlushRxRB();
			a3233_stat = A3233_STAT_RCVNONCE;
			break;

		case A3233_STAT_RCVNONCE:
			nonce_buflen = UART_Read_Cnt();
			if (nonce_buflen >= A3233_NONCE_LEN) {
				UART_Read(nonce_buf, A3233_NONCE_LEN);
				PACK32(nonce_buf, &nonce_value);
				nonce_value = ((nonce_value >> 24) | (nonce_value << 24) | ((nonce_value >> 8) & 0xff00) | ((nonce_value << 8) & 0xff0000));
				nonce_value -= 0x1000;
				UNPACK32(nonce_value, nonce_buf);
				memset(&ret_pkg,0,AVA2_P_COUNT);
				memcpy(ret_pkg+21,nonce_buf,4);
				avalon_init_pkg(ret_pkg,AVA2_P_NONCE);
				UCOM_Write(ret_pkg, AVA2_P_COUNT);

#ifdef A3233_FREQ_DEBUG
				{
					char freq[20];

					m_sprintf(freq, "%04d%04d%04d%04d", AVALON_A3233_FreqNeeded(), AVALON_I2C_TemperRd(), (int)AVALON_A3233_IsTooHot(), AVALON_A3233_ADCGuard(0));
					UCOM_Write(freq, 16);
				}
#endif
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_WAITICA;
				break;
			}

			if (!timestart) {
				AVALON_TMR_Set(A3233_TIMER_TIMEOUT, 2000, NULL);
				timestart = TRUE;
			}

			if (AVALON_TMR_IsTimeout(A3233_TIMER_TIMEOUT)) {
				/* no nonce response */
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_IDLE;
				break;
			}

			if (UCOM_Read_Cnt()) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_CHKICA;
			}

			break;

		default:
			a3233_stat = A3233_STAT_IDLE;
			break;
		}
		/* Sleep until next IRQ happens */
		__WFI();
	}
}

