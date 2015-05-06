/*
===============================================================================
 Name        : avalon_timer.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon timer api
===============================================================================
*/
#include "avalon_api.h"

/* 1ms */
#define TICKRATE_AVALON (1000)

typedef struct {
	Bool 			Enable;
	unsigned int 	CurVal;
	unsigned int 	InitVal;
	TMRPROC			TmrFun;
	Bool			IsTimeout;
}avalon_timer;

avalon_timer tmrlist[AVALON_TMR_MAX];

void TIMER32_0_IRQHandler(void)
{
	AVALON_TMR_e	id;

	if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
		for(id = AVALON_TMR_ID1; id < AVALON_TMR_MAX; id++){
			if (tmrlist[id].Enable && tmrlist[id].CurVal) {
				tmrlist[id].CurVal--;
				if (!tmrlist[id].CurVal) {
					if (tmrlist[id].TmrFun)
						tmrlist[id].TmrFun();
					else
						tmrlist[id].IsTimeout = TRUE;
					tmrlist[id].CurVal = tmrlist[id].InitVal;
				}
			}
		}
	}
}

void AVALON_TMR_Init(void)
{
	AVALON_TMR_e 	id;
	uint32_t		timer_freq;

	for (id=AVALON_TMR_ID1; id<AVALON_TMR_MAX; id++) {
		tmrlist[id].Enable = FALSE;
		tmrlist[id].CurVal = 0;
		tmrlist[id].InitVal = 0;
		tmrlist[id].TmrFun = NULL;
		tmrlist[id].IsTimeout = FALSE;
	}

	/* Enable timer 1 clock */
	Chip_TIMER_Init(LPC_TIMER32_0);

	/* Timer rate is system clock rate */
	timer_freq = Chip_Clock_GetSystemClockRate();

	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(LPC_TIMER32_0);
	Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);
	Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, (timer_freq / TICKRATE_AVALON));
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_0, 1);

	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
	NVIC_EnableIRQ(TIMER_32_0_IRQn);
	Chip_TIMER_Enable(LPC_TIMER32_0);
}

/* tmrcb must be run little time or else use AVALON_TMR_GetReady */
void AVALON_TMR_Set(AVALON_TMR_e id, unsigned int interval, TMRPROC tmrcb)
{
	if ((id >= AVALON_TMR_ID1) && (id < AVALON_TMR_MAX)) {
		tmrlist[id].Enable = TRUE;
		tmrlist[id].CurVal = interval;
		tmrlist[id].InitVal = interval;
		tmrlist[id].TmrFun = tmrcb;
		tmrlist[id].IsTimeout = FALSE;
	}
}

void AVALON_TMR_Kill(AVALON_TMR_e id)
{
	if ((id >= AVALON_TMR_ID1) && (id < AVALON_TMR_MAX)) {
		if (tmrlist[id].Enable)
			tmrlist[id].Enable = FALSE;
	}
}

AVALON_TMR_e AVALON_TMR_GetReady(void)
{
	static AVALON_TMR_e id = AVALON_TMR_ID1;

	if (id == AVALON_TMR_MAX)
		id = AVALON_TMR_ID1;

	for (; id < AVALON_TMR_MAX; id++) {
		if (tmrlist[id].Enable && tmrlist[id].IsTimeout) {
			tmrlist[id].IsTimeout = FALSE;
			break;
		}
	}
	return id;
}

Bool AVALON_TMR_IsTimeout(AVALON_TMR_e id)
{
	if ((id >= AVALON_TMR_ID1) && (id < AVALON_TMR_MAX)) {
		if(tmrlist[id].Enable && tmrlist[id].IsTimeout) {
			tmrlist[id].IsTimeout = FALSE;
			return TRUE;
		}
	}

	return FALSE;
}

static void AVALON_TMRID1_Fun(void)
{
	char str[20];
	m_sprintf(str,"%s\n", __FUNCTION__);
	AVALON_USB_PutSTR(str);
}

static void AVALON_TMRID2_Fun(void)
{
	char str[20];
	m_sprintf(str,"%s\n", __FUNCTION__);
	AVALON_USB_PutSTR(str);
	AVALON_TMR_Kill(AVALON_TMR_ID2);
}

static void AVALON_TMRID3_Fun(void)
{
	char str[20];
	m_sprintf(str,"%s\n", __FUNCTION__);
	AVALON_USB_PutSTR(str);
}

static void AVALON_TMRID4_Fun(void)
{
	char str[20];
	m_sprintf(str,"%s\n", __FUNCTION__);
	AVALON_USB_PutSTR(str);
	AVALON_TMR_Kill(AVALON_TMR_ID4);
}

TMRPROC tmrfun[] = {
	AVALON_TMRID1_Fun,
	AVALON_TMRID2_Fun,
	AVALON_TMRID3_Fun,
	AVALON_TMRID4_Fun
};

/* loop */
void AVALON_TMR_Test(void)
{
	AVALON_TMR_e id;

	AVALON_TMR_Init();

	/* loop callback timer */
	AVALON_TMR_Set(AVALON_TMR_ID1, 1000, tmrfun[AVALON_TMR_ID1]);

	/* once callback timer */
	AVALON_TMR_Set(AVALON_TMR_ID2, 2000, tmrfun[AVALON_TMR_ID2]);

	/* loop polling timer */
	AVALON_TMR_Set(AVALON_TMR_ID3, 3000, NULL);

	/* once pooling timer */
	AVALON_TMR_Set(AVALON_TMR_ID4, 4000, NULL);

	while (1) {
		id = AVALON_TMR_GetReady();
		if (id != AVALON_TMR_MAX)
			tmrfun[id]();
	}
}
