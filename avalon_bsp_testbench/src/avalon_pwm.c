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

void AVALON_Pwm_Init(void)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT16B0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, IOCON_FUNC2 | IOCON_MODE_INACT);//red
	*(unsigned int *)0x4000c004 = 0x1;//enable tcr, ct16b0
	*(unsigned int *)0x4000c008 = 0xffff;//timer counter
	*(unsigned int *)0x4000c03c = 0x1 | (0x3<<8);//External Match
	*(unsigned int *)0x4000c074 = 0x4;//pwm
	*(unsigned int *)0x4000c020 = 0xffff/4;
}

void AVALON_Pwm_Test(void)
{
	static Bool bPwmInit = FALSE;

	if(!bPwmInit)
	{
		bPwmInit = TRUE;
		AVALON_Pwm_Init();
	}
}
