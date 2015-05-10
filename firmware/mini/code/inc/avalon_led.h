/*
 * @brief led head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_LED_H_
#define __AVALON_LED_H_

#define LED_GREEN	0xff00
#define LED_RED	0xff0000
#define LED_BLUE	0xff
#define LED_BLACK	0

void led_init(void);
void led_rgb(unsigned int rgb);
void led_blink(unsigned int rgb);

#endif /* __AVALON_LED_H_ */
