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

void AVALON_Led_Init(void)
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 17);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 15);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);
}

void AVALON_Led_Rgb(unsigned int rgb, Bool on)
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

	default:
		break;
	}
}

void AVALON_LED_Test(void)
{
    /* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	AVALON_Led_Init();

	/* open all led */
	AVALON_Led_Rgb(AVALON_LED_RED, AVALON_LED_ON);
	AVALON_Led_Rgb(AVALON_LED_GREEN, AVALON_LED_ON);
	AVALON_Led_Rgb(AVALON_LED_BLUE, AVALON_LED_ON);
	AVALON_Delay(4000000);

	/* close all led */
	AVALON_Led_Rgb(AVALON_LED_RED, AVALON_LED_OFF);
	AVALON_Led_Rgb(AVALON_LED_GREEN, AVALON_LED_OFF);
	AVALON_Led_Rgb(AVALON_LED_BLUE, AVALON_LED_OFF);
	AVALON_Delay(4000000);

	/* open separate led and close it, r->g->b */
	AVALON_Led_Rgb(AVALON_LED_RED, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_Led_Rgb(AVALON_LED_RED, AVALON_LED_OFF);

	AVALON_Led_Rgb(AVALON_LED_GREEN, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_Led_Rgb(AVALON_LED_GREEN, AVALON_LED_OFF);

	AVALON_Led_Rgb(AVALON_LED_BLUE, AVALON_LED_ON);
	AVALON_Delay(4000000);
	AVALON_Led_Rgb(AVALON_LED_BLUE, AVALON_LED_OFF);

	/* all led has been closed */
	AVALON_Delay(4000000);
}
