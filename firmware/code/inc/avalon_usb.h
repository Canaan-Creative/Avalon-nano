/*
 * @brief A3222 head file
 *
 * @note
 *
 * @par
 */
#ifndef AVALON_USB_H_
#define AVALON_USB_H_

/* debug printf */
void AVALON_USB_Init(void);
void AVALON_USB_PutChar(char ch);
void AVALON_USB_PutSTR(char *str);
void AVALON_USB_Test(void);

#endif /* AVALON_USB_H_ */
