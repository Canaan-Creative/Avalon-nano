/*
 * @brief hid uart head file
 *
 * @note
 *
 * @par
 */
#ifndef __HID_UART_H_
#define __HID_UART_H_

#include "app_usbd_cfg.h"

void uart_init(void);
int hid_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR *pIntfDesc, USBD_API_INIT_PARAM_T *pUsbParam);
uint32_t hid_rxrb_cnt(void);
uint32_t hid_read(uint8_t *pbuf, uint32_t buf_len);
uint32_t hid_write(uint8_t *pbuf, uint32_t len);
void hid_flush_rxrb(void);
uint32_t uart_rxrb_cnt(void);
uint32_t uart_read(uint8_t *pbuf, uint32_t buf_len);
uint32_t uart_write(uint8_t *pbuf, uint32_t len);
void uart_flush_txrb(void);
void uart_flush_rxrb(void);

#endif /* __HID_UART_H_ */
