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

#define VOL_COUNT 2

#define PG_COUNT  2
#define PG_BAD    0x02
#define PG_GOOD   0x01

#define VCORE_OFF 0x100

void vcore_init(void);
uint8_t  set_voltage(uint16_t vol);
uint16_t get_voltage(void);
void vcore_disable(uint8_t num);
void vcore_enable(uint8_t num);
void vcore_detect(void);
uint8_t get_pg_state(uint8_t pg_id);

#endif /* __AVALON_VCORE_H_ */
