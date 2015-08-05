/*
 * @brief defines head file
 *
 * @note
 *
 * @par
 */
#ifndef __DEFINES_H_
#define __DEFINES_H_

#define IDLE_TIME	3000 /* 3s */
#define WDT_IDLE	3 /* 3s */
#define ASIC_COUNT		5
#define ASIC_0V                 0xff
#define ASIC_CORETEST_VOLT	0x93	/* 0.7000 */
#define ASIC_CORETEST_FREQ	0x1e278447	/* 200 */
#define TEST_CORE_COUNT	16
#define ADC_CUT	200	/* 66 ℃ */
#define ADC_RESUME	230 /* 60 ℃ */

#ifdef DEBUG
#include "avalon_uart.h"
#define debug32(...)	do {				\
		char printf_buf32[32];	\
		sprintf(printf_buf32, __VA_ARGS__);	\
		uart_puts(printf_buf32);		\
	} while(0)
#else
#define debug32(...)
#endif

#endif /* __DEFINES_H_ */

