/*
===============================================================================
 Name        : avalon_led.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon led api
===============================================================================
*/

#include "avalon_api.h"

void AVALON_LED_Init(void)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 17);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 15);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);

	Board_LED_Set(0, 1);
	Board_LED_Set(1, 1);
	Board_LED_Set(2, 1);
}

void AVALON_LED_Rgb(unsigned int rgb, Bool on)
{
	switch(rgb){
	case AVALON_LED_GREEN:
		Board_LED_Set(0, on);//green
		break;

	case AVALON_LED_RED:
		Board_LED_Set(1, on);//red
		break;

	case AVALON_LED_BLUE:
		Board_LED_Set(2, on);//blue
		break;

	case AVALON_LED_ALL:
		Board_LED_Set(0, on);
		Board_LED_Set(1, on);
		Board_LED_Set(2, on);
		break;

	default:
		break;
	}
}

void AVALON_LED_Test(void)
{
    /* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	AVALON_LED_Init();

	/* open all led */
	AVALON_LED_Rgb(AVALON_LED_RED, AVALON_LED_ON);
	AVALON_LED_Rgb(AVALON_LED_GREEN, AVALON_LED_ON);
	AVALON_LED_Rgb(AVALON_LED_BLUE, AVALON_LED_ON);
	AVALON_Delay(4000000);

	/* close all led */
	AVALON_LED_Rgb(AVALON_LED_RED, AVALON_LED_OFF);
	AVALON_LED_Rgb(AVALON_LED_GREEN, AVALON_LED_OFF);
	AVALON_LED_Rgb(AVALON_LED_BLUE, AVALON_LED_OFF);
	AVALON_Delay(4000000);

	/* open separate led and close it, r->g->b */
	AVALON_LED_Rgb(AVALON_LED_RED, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_LED_Rgb(AVALON_LED_RED, AVALON_LED_OFF);

	AVALON_LED_Rgb(AVALON_LED_GREEN, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_LED_Rgb(AVALON_LED_GREEN, AVALON_LED_OFF);

	AVALON_LED_Rgb(AVALON_LED_BLUE, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_LED_Rgb(AVALON_LED_BLUE, AVALON_LED_OFF);

	/* all led has been closed */
	AVALON_Delay(4000000);
}
