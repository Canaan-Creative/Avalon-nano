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
#include "lpc_types.h"
#include "board.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* led rgb index */
#define AVALON_LED_GREEN 	0
#define AVALON_LED_RED		1
#define AVALON_LED_BLUE		2
#define AVALON_LED_ALL		3

#define AVALON_LED_ON		(FALSE)
#define AVALON_LED_OFF		(TRUE)

typedef void (*TMRPROC)(void);

typedef enum{
	AVALON_TMR_ID1,
	AVALON_TMR_ID2,
	AVALON_TMR_ID3,
	AVALON_TMR_ID4,
	AVALON_TMR_MAX
}AVALON_TMR_e;

/* led */
void AVALON_LED_Init(void);
void AVALON_LED_Rgb(unsigned int rgb, Bool on);
void AVALON_LED_Test(void);

/* debug printf */
void AVALON_USB_Init(void);
void AVALON_USB_PutChar(char ch);
void AVALON_USB_PutSTR(char *str);
void AVALON_USB_Test(void);

/* adc */
void AVALON_ADC_Init(void);
void AVALON_ADC_Rd(uint8_t channel, uint16_t *data);

/* iic */
void AVALON_I2C_Init(void);
unsigned int AVALON_I2C_TemperRd();

/* a3233 */
void AVALON_A3233_Init(void);
void AVALON_A3233_Test(void);

/* pwm */
void AVALON_PWM_Init(void);
void AVALON_PWM_Test(void);

/* timer */
void AVALON_TMR_Init(void);
void AVALON_TMR_Set(AVALON_TMR_e id, unsigned int interval, TMRPROC tmrcb);
void AVALON_TMR_Kill(AVALON_TMR_e id);
AVALON_TMR_e AVALON_TMR_GetReady(void);
void AVALON_TMR_Test(void);

/* watchdog */
void AVALON_WDT_Init(void);
void AVALON_WDT_Enable(void);
void AVALON_WDT_Feed(void);
void AVALON_WDT_Test(void);

static void AVALON_Delay(unsigned int max)
{
	volatile unsigned int i;
	for(i = 0; i < max; i++);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __AVALON_API_H_ */
