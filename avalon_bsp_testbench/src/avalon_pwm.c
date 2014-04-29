/*
===============================================================================
 Name        : avalon_pwm.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon pwm api
===============================================================================
*/
#include "avalon_api.h"

#define DUTY_100		(uint32_t)(60000)
#define DUTY_50			(uint32_t)(DUTY_100*0.5)
#define DUTY_25			(uint32_t)(DUTY_100*0.75)
#define DUTY_10			(uint32_t)(DUTY_100*0.9)

void AVALON_Pwm_Init(void)
{
	/* System CLK 48MHz */
	Chip_TIMER_Init(LPC_TIMER16_0);
	Chip_TIMER_Disable(LPC_TIMER16_0);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 Init */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, IOCON_FUNC2 | IOCON_MODE_INACT);

	/* CT16B0_MAT0 duty:50% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_TOGGLE, 0);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, DUTY_50);

	/* CT16B0_MAT1 duty:25% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_TOGGLE, 1);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, DUTY_25);

	/* CT16B0_MAT2 duty:10% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_TOGGLE, 2);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 2, DUTY_10);

	/* Prescale 0 */
	Chip_TIMER_PrescaleSet(LPC_TIMER16_0, 0);

	/* PWM Period 800Hz */
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 3, DUTY_100);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 3);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 Enable */
	LPC_TIMER16_0->PWMC = 0b111;//pwm

	Chip_TIMER_Enable(LPC_TIMER16_0);
}


void AVALON_Pwm_Test(void)
{
	static Bool bPwmInit = FALSE;

	if(!bPwmInit)
	{
		bPwmInit = TRUE;
		AVALON_Pwm_Init();
	}
	Chip_TIMER_Enable(LPC_TIMER16_0);
	AVALON_Delay(9000000);
	Chip_TIMER_Disable(LPC_TIMER16_0);
}
