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

#define LED_12V     0x00
#define LED_12V_1T  0x01
#define LED_12V_1F  0x02
#define LED_12V_2T  0x04
#define LED_12V_2F  0x08

#define TIMER_LED_12V    TIMER_ID3
#define TIMER_LED_12V_1T TIMER_ID4
#define TIMER_LED_12V_1F TIMER_ID5
#define TIMER_LED_12V_2T TIMER_ID6
#define TIMER_LED_12V_2F TIMER_ID7

void led_init(void);
void led_rgb(unsigned int rgb, unsigned int state);
void led_blink_on(unsigned int led);
void led_blink_off(unsigned int led);

#endif /* __AVALON_LED_H_ */
