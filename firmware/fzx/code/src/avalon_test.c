/*
 * @brief
 *
 * @note
 * Author: Fanzixiao fanzixiao@canaan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#ifdef DEBUG

#include <string.h>
#include "board.h"

#include "defines.h"
#include "libfunctions.h"
#include "avalon_test.h"


void test_system_clock_out(int clock_select)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, IOCON_FUNC1);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 1);

	switch (clock_select) {
	case 0:
		Chip_Clock_SetCLKOUTSource(SYSCTL_MAINCLKSRC_IRC,1);
		break;
	case 1:
		Chip_Clock_SetCLKOUTSource(SYSCTL_MAINCLKSRC_PLLIN,1);
		break;
	case 2:
		Chip_Clock_SetCLKOUTSource(SYSCTL_MAINCLKSRC_WDTOSC,1);
		break;
	case 3:
		Chip_Clock_SetCLKOUTSource(SYSCTL_MAINCLKSRC_PLLOUT,1);
		break;
	default:
		Chip_Clock_SetCLKOUTSource(SYSCTL_MAINCLKSRC_PLLOUT,1);
		break;
	}
}

#endif
