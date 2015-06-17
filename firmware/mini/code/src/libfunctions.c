/*
 * @brief public functions
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "libfunctions.h"

/* FIXME: SystemCoreClock / 10000 is not accurate */
#define TICKS (48000000 / 10000)

void delay(unsigned int ms)
{
	unsigned int i;

	while (ms && ms--) {
		for(i = 0; i < TICKS; i++)
			__NOP();
	}
}
