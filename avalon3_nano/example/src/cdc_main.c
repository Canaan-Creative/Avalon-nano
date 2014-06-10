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
#include <NXP/crp.h>
#include "sha2.h"

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

__CRP unsigned int CRP_WORD = CRP_NO_ISP;
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
static unsigned char 	golden_ob[] = "\x46\x79\xba\x4e\xc9\x98\x76\xbf\x4b\xfe\x08\x60\x82\xb4\x00\x25\x4d\xf6\xc3\x56\x45\x14\x71\x13\x9a\x3a\xfa\x71\xe4\x8f\x54\x4a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x87\x32\x0b\x1a\x14\x26\x67\x4f\x2f\xa7\x22\xce";
static unsigned int 	a3233_stat = A3233_STAT_WAITICA;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int main(void)
{
	uint8_t 		icarus_buf[ICA_TASK_LEN];
	unsigned int	icarus_buflen = 0;
	unsigned char 	work_buf[A3233_TASK_LEN];
	unsigned char	nonce_buf[A3233_NONCE_LEN];
	unsigned int	nonce_buflen = 0;
	unsigned int	last_freq = 0;
	uint32_t 		nonce_value = 0;
	Bool			isgoldenob = FALSE;
	Bool			timestart = FALSE;

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
			if (icarus_buflen >= ICA_TASK_LEN) {
				timestart = FALSE;
				AVALON_TMR_Kill(A3233_TIMER_TIMEOUT);
				a3233_stat = A3233_STAT_PROCICA;
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

		case A3233_STAT_PROCICA:
			memset(icarus_buf, 0, ICA_TASK_LEN);
			UCOM_Read(icarus_buf, ICA_TASK_LEN);
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

				UCOM_Write(nonce_buf, A3233_NONCE_LEN);

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

