/*
 * @brief avalon routines
 *
 * @note
 * Copyright(C) canaan creative, 2015
 * All rights reserved.
 *
 */
#ifndef __AVALON_API_H_
#define __AVALON_API_H_

#include <stdio.h>
#include "board.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MM_TASK_LEN        64

#define A3222_REPORT_SIZE       12 /* work_id (8 bytes) + nonce (4bytes) */
#define A3222_REPORT_NONCE_CNT  55

typedef void (*TMRPROC)(void);

typedef enum {
	AVALON_TMR_ID1,
	AVALON_TMR_ID2,
	AVALON_TMR_ID3,
	AVALON_TMR_ID4,
	AVALON_TMR_MAX
} AVALON_TMR_e;

/* timer */
void AVALON_TMR_Init(void);
void AVALON_TMR_Set(AVALON_TMR_e id, unsigned int interval, TMRPROC tmrcb);
void AVALON_TMR_Kill(AVALON_TMR_e id);
AVALON_TMR_e AVALON_TMR_GetReady(void);
Bool AVALON_TMR_IsTimeout(AVALON_TMR_e id);
unsigned int AVALON_TMR_Elapsed(AVALON_TMR_e id);
void AVALON_TMR_Test(void);

/* debug printf */
void AVALON_USB_Init(void);
void AVALON_USB_PutChar(char ch);
void AVALON_USB_PutSTR(char *str);
void AVALON_USB_Test(void);

/* printf */
char *m_sprintf(char *dest, const char *format, ...);

void AVALON_Delay(unsigned int ms);

/* A3222 */
void AVALON_A3222_Init(void);
void AVALON_A3222_Process(uint8_t *work, const uint32_t *work_id);
uint8_t AVALON_A3222_ReportCnt(void);
uint8_t AVALON_A3222_GetReport(uint8_t *report);

#ifdef __cplusplus
}
#endif

#endif /* __AVALON_API_H_ */
