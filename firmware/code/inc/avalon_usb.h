/*
 * avalon_usb.h
 *
 *  Created on: Apr 27, 2015
 *      Author: xiangfu
 */

#ifndef AVALON_USB_H_
#define AVALON_USB_H_

/* debug printf */
void AVALON_USB_Init(void);
void AVALON_USB_PutChar(char ch);
void AVALON_USB_PutSTR(char *str);
void AVALON_USB_Test(void);

#endif /* AVALON_USB_H_ */
