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
#ifndef __CDC_AVALON_H_
#define __CDC_AVALON_H_

#include <stdio.h>
#include "board.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AVALON_LED_GREEN 	0
#define AVALON_LED_RED		1
#define AVALON_LED_BLUE		2
#define AVALON_LED_OFF		3

/** @ingroup EXAMPLES_USBDLIB_11XX_CDC_UART
 * @{
 */

/**
 * @brief	USB to UART bridge port init routine
 * @param	pDesc		: Pointer to configuration descriptor
 * @param	pUsbParam	: Pointer USB param structure returned by previous init call
 * @return	Always returns LPC_OK.
 */
ErrorCode_t AVALON_init (void);

/*
 * */
void AVALON_led_rgb(unsigned int rgb);

void AVALON_POWER_Enable(bool On);

void AVALON_Rstn_A3233();

unsigned int AVALON_Gen_A3233_Pll_Cfg(unsigned int freq);

void AVALON_Delay(unsigned int max);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CDC_AVALON_H_ */
