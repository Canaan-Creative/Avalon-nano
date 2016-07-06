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

#define PG1       0x01
#define PG2       0x02

void vcore_init(void);
uint8_t  set_voltage(uint16_t vol);
uint16_t get_voltage(void);
void vcore_disable(uint8_t num);
void vcore_enable(uint8_t num);
void vcore_detect(void);
uint8_t get_pg_flag(void);

#endif /* __AVALON_VCORE_H_ */
