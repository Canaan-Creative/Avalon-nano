/*
 * @brief WDT head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_WDT_H_
#define __AVALON_WDT_H_

void wdt_init(uint8_t second);
void wdt_enable(void);
void wdt_feed(void);

#endif /* __AVALON_WDT_H_ */

