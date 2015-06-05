/*
 * @brief A3222 head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_A3222_H_
#define __AVALON_A3222_H_

#define A3222_DEFAULT_FREQ  0x1e678447  /* 100 Mhz */

void a3222_reset(void);
void a3222_hw_init(void);
void a3222_sw_init(void);
void a3222_roll_work(uint8_t *pkg, int ntime_offset);
int a3222_push_work(uint8_t *pkg);
int a3222_process(void);

int a3222_get_works_count(void);
int a3222_get_report_count(void);
int a3222_get_report(uint8_t *pkg);

void a3222_set_freq(uint32_t freq[], uint8_t index);
void a3222_get_freq(uint32_t freq[], uint8_t index);

void a3222_set_spispeed(uint32_t speed);
uint32_t a3222_get_spispeed(void);

void a3222_set_ntime(uint8_t ntime);
#endif /* __AVALON_A3222_H_ */
