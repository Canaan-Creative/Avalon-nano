/*
 * @brief avalon routines
 *
 * @note
 * Copyright(C) canaan creative, 2015
 * All rights reserved.
 *
 */
#ifndef __AVALON_A3222_H_
#define __AVALON_A3222_H_

#include <stdint.h>

#define A3222_WORK_SIZE		(23*4)
#define A3222_REPORT_SIZE	12 /* work_id (8 bytes) + nonce (4bytes) */
#define A3222_REPORT_CNT	48

void Init(void);
void Process(uint8_t *work, const uint32_t *work_id);
uint8_t ReportCnt(void);
uint8_t GetReport(uint8_t *report);

#endif /* __AVALON_A3222_H_ */
