/*
 * @brief shifter head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_VCORE_H_
#define __AVALON_VCORE_H_

void vcore_init(void);
uint8_t  set_voltage(uint16_t vol);
uint16_t get_voltage(void);

#endif /* __AVALON_VCORE_H_ */
