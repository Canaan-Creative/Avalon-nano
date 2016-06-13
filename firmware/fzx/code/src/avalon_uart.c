/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#include <string.h>
#include "board.h"
#include "avalon_uart.h"

static void init_pin(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, IOCON_FUNC1 | IOCON_MODE_INACT);	/* PIO0_18 used for RXD */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, IOCON_FUNC1 | IOCON_MODE_INACT);	/* PIO0_19 used for TXD */
}

void uart_init(void)
{
	init_pin();

	Chip_UART_Init(LPC_USART);
	Chip_UART_SetBaud(LPC_USART, 115200);
	Chip_UART_ConfigData(LPC_USART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_USART);
}

void uart_puts(const char *str)
{
	Chip_UART_SendBlocking(LPC_USART, str, strlen(str));
}

void uart_nwrite(const char *str, unsigned int len)
{
	Chip_UART_SendBlocking(LPC_USART, str, len);
}

int uart_read(char *ch)
{
	char data;

	if(Chip_UART_Read(LPC_USART, &data, 1) == 1) {
		*ch = data;
		return 0;
	}

	return 1;
}
