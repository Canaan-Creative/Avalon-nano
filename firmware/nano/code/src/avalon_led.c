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

#define A3233_TIMER_LED				(AVALON_TMR_ID2)

static unsigned int blinkcolor;

/*make sure timer has init*/
void AVALON_LED_Init(void)
{
	AVALON_PWM_Init();
}

static void AVALON_LED_PWMSet(unsigned int rgb)
{
	unsigned char r,g,b;

	r = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	b = (rgb & 0xff);

	/*FIXME:PWM set need delay some times?*/
	AVALON_PWM_Disable();
	AVALON_PWM_SetDuty(AVALON_PWM_RED, r);
	AVALON_PWM_SetDuty(AVALON_PWM_GREEN, g);
	AVALON_PWM_SetDuty(AVALON_PWM_BLUE, b);
	AVALON_PWM_Enable();
}

void AVALON_LED_Rgb(unsigned int rgb)
{
	AVALON_TMR_Kill(A3233_TIMER_LED);
	AVALON_LED_PWMSet(rgb);
}

static void AVALON_LED_Blinkcb(void)
{
	static Bool is_open = FALSE;

	if (is_open) {
		AVALON_LED_PWMSet(AVALON_LED_BLACK);
		is_open = FALSE;
	} else {
		AVALON_LED_PWMSet(blinkcolor);
		is_open = TRUE;
	}
}

void AVALON_LED_Blink(unsigned int rgb)
{
	blinkcolor = rgb;
	AVALON_TMR_Set(A3233_TIMER_LED, 500, AVALON_LED_Blinkcb);
}

void AVALON_LED_Test(void)
{
    /* Initialize GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	AVALON_LED_Init();

	/* open all led */
	AVALON_LED_Rgb(AVALON_LED_WHITE);
	AVALON_Delay(1000);

	/* close all led */
	AVALON_LED_Rgb(AVALON_LED_BLACK);
	AVALON_Delay(1000);

	/* open separate led and close it, r->g->b */
	AVALON_LED_Rgb(AVALON_LED_RED);
	AVALON_Delay(1000);
	AVALON_LED_Rgb(AVALON_LED_BLACK);

	AVALON_LED_Rgb(AVALON_LED_GREEN);
	AVALON_Delay(1000);
	AVALON_LED_Rgb(AVALON_LED_BLACK);

	AVALON_LED_Rgb(AVALON_LED_BLUE);
	AVALON_Delay(1000);
	AVALON_LED_Rgb(AVALON_LED_BLACK);

	/* all led has been closed */
	AVALON_Delay(1000);

	AVALON_LED_Blink(AVALON_LED_RED);
	AVALON_Delay(4000);
}
