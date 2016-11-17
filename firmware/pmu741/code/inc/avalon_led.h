/*
 * @brief led head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_LED_H_
#define __AVALON_LED_H_

#define LED_COUNT   2

#define LED_ON      1
#define LED_OFF     0

#define LED_GREEN   0x01
#define LED_RED     0x02

#define LED_GREEN_BLINK 0x04
#define LED_RED_BLINK   0x08

#define LED_12V_1T  0x01
#define LED_12V_1F  0x02
#define LED_12V_2T  0x04
#define LED_12V_2F  0x08
#define LED_12V_1TF 0x10
#define LED_12V_2TF 0x20

#define TIMER_LED_12V_1T  TIMER_ID3
#define TIMER_LED_12V_1F  TIMER_ID4
#define TIMER_LED_12V_2T  TIMER_ID5
#define TIMER_LED_12V_2F  TIMER_ID6
#define TIMER_LED_12V_1TF TIMER_ID7
#define TIMER_LED_12V_2TF TIMER_ID8

void led_init(void);
void set_led_state(uint16_t state);
uint16_t get_led_state(uint8_t led_id);

#endif /* __AVALON_LED_H_ */
