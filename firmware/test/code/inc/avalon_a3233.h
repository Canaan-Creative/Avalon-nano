/*
 * @brief a3233 head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_A3233_H_
#define __AVALON_A3233_H_

void a3233_init(void);
void a3233_enable_power(bool on);
bool a3233_power_isenable(void);
void a3233_reset_asic(void);
int a3233_push_work(uint8_t *pkg);
int a3222_get_nonce(uint32_t *nonce);
void a3233_set_freq(uint32_t freq);

#endif /* __AVALON_A3233_H_ */

