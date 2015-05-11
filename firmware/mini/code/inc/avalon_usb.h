/*
 * @brief A3222 head file
 *
 * @note
 *
 * @par
 */
#ifndef AVALON_USB_H_
#define AVALON_USB_H_

void usb_init(void);
void USB_IRQHandler(void);
void usb_reconnect(void);

#endif /* AVALON_USB_H_ */
