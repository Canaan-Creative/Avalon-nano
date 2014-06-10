/*
 * @brief Programming API used with avalon
 *
 * @note
 * Copyright(C) 0xf8, 2014
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
#ifndef __AVALON_API_H_
#define __AVALON_API_H_

#include <stdio.h>
#include "board.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AVALON_LED_GREEN 	0xff00
#define AVALON_LED_RED		0xff0000
#define AVALON_LED_BLUE		0xff
#define AVALON_LED_BLACK	0
#define AVALON_LED_WHITE	0xffffff

typedef void (*TMRPROC)(void);

typedef enum {
	AVALON_TMR_ID1,
	AVALON_TMR_ID2,
	AVALON_TMR_ID3,
	AVALON_TMR_ID4,
	AVALON_TMR_MAX
} AVALON_TMR_e;

typedef enum {
	AVALON_PWM_GREEN,
	AVALON_PWM_BLUE,
	AVALON_PWM_RED,
	AVALON_PWM_MAX
} AVALON_PWM_e;

/** @ingroup EXAMPLES_USBDLIB_11XX_CDC_UART
 * @{
 */

/**
 * @brief	USB to UART bridge port init routine
 * @param	pDesc		: Pointer to configuration descriptor
 * @param	pUsbParam	: Pointer USB param structure returned by previous init call
 * @return	Always returns LPC_OK.
 */
ErrorCode_t AVALON_Init (void);

/*
 * */
void AVALON_POWER_Enable(Bool On);
Bool AVALON_POWER_IsEnable(void);
void AVALON_Rstn_A3233();
unsigned int AVALON_Gen_A3233_Pll_Cfg(unsigned int freq, unsigned int *actfreq);
void AVALON_Delay(unsigned int max);
unsigned int A3233_FreqNeeded();
unsigned int A3233_FreqMin(void);
unsigned int A3233_FreqMax(void);
Bool A3233_IsTooHot(void);

/* timer */
void AVALON_TMR_Init(void);
void AVALON_TMR_Set(AVALON_TMR_e id, unsigned int interval, TMRPROC tmrcb);
void AVALON_TMR_Kill(AVALON_TMR_e id);
AVALON_TMR_e AVALON_TMR_GetReady(void);
Bool AVALON_TMR_IsTimeout(AVALON_TMR_e id);
void AVALON_TMR_Test(void);

/* pwm */
void AVALON_PWM_Init(void);
void AVALON_PWM_SetDuty(AVALON_PWM_e pwm, unsigned char duty);
void AVALON_PWM_Enable(void);
void AVALON_PWM_Disable(void);
void AVALON_PWM_Test(void);

/* led */
void AVALON_LED_Init(void);
void AVALON_LED_Rgb(unsigned int rgb);
void AVALON_LED_Blink(unsigned int rgb);
void AVALON_LED_Test(void);

/* debug printf */
void AVALON_USB_Init(void);
void AVALON_USB_PutChar(char ch);
void AVALON_USB_PutSTR(char *str);
void AVALON_USB_Test(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __AVALON_API_H_ */
