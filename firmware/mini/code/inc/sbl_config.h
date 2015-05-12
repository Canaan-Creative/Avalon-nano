/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    SBL_CONFIG.H
 *      Purpose: USB Flash updater
 *      Version: V1.0
 *----------------------------------------------------------------------------
 * Software that is described herein is for illustrative purposes only  
 * which provides customers with programming information regarding the  
 * products. This software is supplied "AS IS" without any warranties.  
 * NXP Semiconductors assumes no responsibility or liability for the 
 * use of the software, conveys no license or title under any patent, 
 * copyright, or mask work right to the product. NXP Semiconductors 
 * reserves the right to make changes in the software without 
 * notification. NXP Semiconductors also make no representation or 
 * warranty that such application will be suitable for the specified 
 * use without further testing or modification. 
 *---------------------------------------------------------------------------*/

#ifndef  _SBL_CONFIG_H
#define  _SBL_CONFIG_H

#define CCLK 48000 					/* 48,000 KHz for IAP call */

#define FLASH_BUF_SIZE		512
#define SECTOR_0_START_ADDR 0
#define SECTOR_SIZE			4096
#define MAX_USER_SECTOR		8

#endif  /* __SBL_CONFIG_H__ */
