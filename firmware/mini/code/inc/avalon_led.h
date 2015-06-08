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

#define LED_ON	0
#define LED_OFF	1
#define LED_BREATH	2

void led_init(void);
void led_set(unsigned int led, uint8_t led_op);

#endif /* __AVALON_LED_H_ */
