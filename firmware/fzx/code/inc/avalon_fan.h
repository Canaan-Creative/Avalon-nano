/*
 * @brief led head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_FAN_H_
#define __AVALON_FAN_H_

void fan_init(void);
void fan_enable(void);
void fan_disable(void);
void fan_adjust_pwm(uint8_t pwm_duty);
void fan_measure_speed();
uint32_t fan_get_speed();

#endif /* __AVALON_FAN_H_ */
