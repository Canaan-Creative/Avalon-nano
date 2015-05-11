/*
 * @brief dfu head file
 *
 * @note
 *
 * @par
 */
#ifndef __DFU_H_
#define __DFU_H_

ErrorCode_t dfu_init(USBD_HANDLE_T hUsb, USB_INTERFACE_DESCRIPTOR* pIntfDesc, USBD_API_INIT_PARAM_T *pUsbParam);
uint8_t dfu_sig(void);
void dfu_proc(void);

#endif /* __DFU_H_ */
