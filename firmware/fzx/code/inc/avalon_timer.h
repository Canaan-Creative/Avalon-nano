/*
 * @brief timer head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_TIMER_
#define __AVALON_TIMER_

enum timer_id {
	TIMER_ID1,
	TIMER_ID2,
	TIMER_ID3,
	TIMER_ID4,
	TIMER_MAX
};

void timer_init(void);
void timer_set(enum timer_id id, uint32_t interval);
bool timer_istimeout(enum timer_id id);

#endif /* __AVALON_TIMER_ */
