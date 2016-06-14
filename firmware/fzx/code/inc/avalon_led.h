/*
 * @brief led head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_LED_H_
#define __AVALON_LED_H_

#define LED_ON      1
#define LED_OFF     0

#define LED_12V_1T  0x01
#define LED_12V_1F  0x02
#define LED_12V_2T  0x03
#define LED_12V_2F  0x04

void led_init(void);
void led_set(unsigned int led, unsigned int state);

#endif /* __AVALON_LED_H_ */
