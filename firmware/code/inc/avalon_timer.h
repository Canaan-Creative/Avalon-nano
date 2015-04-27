/*
 * @brief A3222 head file
 *
 * @note
 *
 * @par
 */

#ifndef AVALON_TIMER_H_
#define AVALON_TIMER_H_

typedef void (*TMRPROC)(void);

typedef enum {
	AVALON_TMR_ID1,
	AVALON_TMR_ID2,
	AVALON_TMR_ID3,
	AVALON_TMR_ID4,
	AVALON_TMR_MAX
} AVALON_TMR_e;

/* timer */
void AVALON_TMR_Init(void);
void AVALON_TMR_Set(AVALON_TMR_e id, unsigned int interval, TMRPROC tmrcb);
void AVALON_TMR_Kill(AVALON_TMR_e id);
AVALON_TMR_e AVALON_TMR_GetReady(void);
Bool AVALON_TMR_IsTimeout(AVALON_TMR_e id);
unsigned int AVALON_TMR_Elapsed(AVALON_TMR_e id);
void AVALON_TMR_Test(void);

/* printf */
char *m_sprintf(char *dest, const char *format, ...);

#endif /* AVALON_TIMER_H_ */
