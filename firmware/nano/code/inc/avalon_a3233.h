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
void a3233_asicreset(void);
unsigned int a3233_getpll(unsigned int freq);

#endif /* __AVALON_A3233_H_ */

