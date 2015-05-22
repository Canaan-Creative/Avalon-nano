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
#define ASIC_COUNT		5
#define ASIC_0V                 0xff

#ifdef DEBUG
#include "avalon_uart.h"
char printf_buf32[32];
#define debug32(...)	do {				\
		sprintf(printf_buf32, __VA_ARGS__);	\
		uart_puts(printf_buf32);		\
	} while(0)
#else
#define debug32(...)
#endif

#endif /* __DEFINES_H_ */

