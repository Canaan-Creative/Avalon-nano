/*
 * @brief timer head file
 *
 * @note
 *
 * @par
 */

enum timer_id {
    TIMER_ID1,
    TIMER_MAX
};

void timer_init(void);
void timer_set(enum timer_id id, uint32_t interval);
bool timer_istimeout(enum timer_id id);
