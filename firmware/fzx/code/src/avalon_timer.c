/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *         fanzixiao@canaan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "avalon_timer.h"

/* 1ms */
#define TICKRATE (1000)

static uint32_t timerval[TIMER_MAX];

void TIMER32_0_IRQHandler(void)
{
	enum timer_id	id;

	if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1)) {
		Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
		for (id = TIMER_ID1; id < TIMER_MAX; id++) {
			if (timerval[id])
				timerval[id]--;
		}
	}
}

void timer_init(void)
{
	enum timer_id	id;
	uint32_t	timer_freq;

	for (id = TIMER_ID1; id < TIMER_MAX; id++)
		timerval[id] = 0;

	/* enable timer 1 clock */
	Chip_TIMER_Init(LPC_TIMER32_0);

	/* Timer rate is system clock rate */
	timer_freq = Chip_Clock_GetSystemClockRate();

	/* Timer setup for match and interrupt at TICKRATE */
	Chip_TIMER_Reset(LPC_TIMER32_0);
	Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);
	Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, (timer_freq / TICKRATE));
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_0, 1);

	/* enable timer interrupt */
	NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
	NVIC_EnableIRQ(TIMER_32_0_IRQn);
	Chip_TIMER_Enable(LPC_TIMER32_0);
}

void timer_set(enum timer_id id, uint32_t interval)
{
	timerval[id] = interval;;
}

bool timer_istimeout(enum timer_id id)
{
	if (!timerval[id])
		return true;

	return false;
}

