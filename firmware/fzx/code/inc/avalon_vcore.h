/*
 * @brief shifter head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_VCORE_H_
#define __AVALON_VCORE_H_

#define VCORE1    1
#define VCORE2    2

void vcore_init(void);
uint8_t  set_voltage(uint16_t vol);
uint16_t get_voltage(void);
void vcore_disable(uint8_t num);
void vcore_enable(uint8_t num);

#endif /* __AVALON_VCORE_H_ */
