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

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 17, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 15, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 19, 1);
}

void AVALON_LED_Rgb(unsigned int rgb, Bool on)
{
	switch(rgb){
	case AVALON_LED_GREEN:
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 17, on);//green
		break;

	case AVALON_LED_RED:
		Chip_GPIO_SetPinState(LPC_GPIO, 1, 15, on);//red
		break;

	case AVALON_LED_BLUE:
		Chip_GPIO_SetPinState(LPC_GPIO, 1, 19, on);//blue
		break;

	case AVALON_LED_ALL:
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 17, on);
		Chip_GPIO_SetPinState(LPC_GPIO, 1, 15, on);
		Chip_GPIO_SetPinState(LPC_GPIO, 1, 19, on);
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
