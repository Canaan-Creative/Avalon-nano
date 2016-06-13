/*
 * @brief uart head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_UART_H_
#define __AVALON_UART_H_

void uart_init(void);
void uart_puts(const char *str);
void uart_nwrite(const char *str, unsigned int len);
int uart_read(char *ch);

#endif /* __AVALON_UART_H_ */

