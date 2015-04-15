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

#define DUTY_100		(uint32_t)(256)
#define DUTY_50			(uint32_t)(DUTY_100*0.5)
#define DUTY_25			(uint32_t)(DUTY_100*0.75)
#define DUTY_10			(uint32_t)(DUTY_100*0.9)
#define DUTY_0			(0)

void AVALON_PWM_Init(void)
{
	/* System CLK 48MHz */
	Chip_TIMER_Init(LPC_TIMER16_0);
	Chip_TIMER_Disable(LPC_TIMER16_0);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 Init */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, IOCON_FUNC2 | IOCON_MODE_INACT);

	/* CT16B0_MAT0 duty:50% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 0);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, DUTY_0);

	/* CT16B0_MAT1 duty:25% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 1);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, DUTY_0);

	/* CT16B0_MAT2 duty:10% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 2);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 2, DUTY_0);
}

void AVALON_PWM_SetDuty(AVALON_PWM_e pwm, unsigned char duty)
{
	if ((pwm >= AVALON_PWM_GREEN) && (pwm < AVALON_PWM_MAX)) {
		Chip_TIMER_ClearMatch(LPC_TIMER16_0, pwm);
		Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, (int8_t)pwm);
		Chip_TIMER_SetMatch(LPC_TIMER16_0, pwm, (uint32_t)duty);
	}
}

void AVALON_PWM_Enable(void)
{
	/* Prescale 0 */
	Chip_TIMER_PrescaleSet(LPC_TIMER16_0, 0);

	/* PWM Period 800Hz */
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 3, DUTY_100);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 3);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 Enable */
	LPC_TIMER16_0->PWMC = 0x7;

	Chip_TIMER_Enable(LPC_TIMER16_0);
}

void AVALON_PWM_Disable(void)
{
	Chip_TIMER_Disable(LPC_TIMER16_0);
}

void AVALON_PWM_Test(void)
{
	unsigned char duty = 0;

	AVALON_PWM_Init();
	AVALON_PWM_Enable();
	for (;duty < 255; duty++) {
		AVALON_PWM_SetDuty(AVALON_PWM_RED, duty);
		AVALON_Delay(20);
	}
	AVALON_Delay(1000);
	for (;duty > 0; duty--) {
		AVALON_PWM_SetDuty(AVALON_PWM_RED, duty);
		AVALON_Delay(20);
	}
	AVALON_Delay(1000);
}
