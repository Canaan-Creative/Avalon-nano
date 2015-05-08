/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */

#include "board.h"
#include "avalon_led.h"
#include "avalon_timer.h"

#define PIN_LED_RED	15
#define PIN_LED_GREEN	8
#define PIN_LED_BLUE	9

#define A3233_TIMER_LED	TMR_ID3

static uint32_t blinkcolor;

static void led_setduty(uint8_t r_duty, uint8_t g_duty, uint8_t b_duty)
{
	Chip_TIMER_ClearMatch(LPC_TIMER16_0, 2);
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 2);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 2, r_duty);

	Chip_TIMER_ClearMatch(LPC_TIMER16_0, 0);
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 0);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, g_duty);

	Chip_TIMER_ClearMatch(LPC_TIMER16_0, 1);
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 1);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, b_duty);
}

static void led_set(uint32_t rgb)
{
	uint8_t r, g, b;

	r = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	b = (rgb & 0xff);

	/* FIXME:PWM set need delay some times? */
	Chip_TIMER_Disable(LPC_TIMER16_0);
	led_setduty(r, g, b);

	/* Prescale 0 */
	Chip_TIMER_PrescaleSet(LPC_TIMER16_0, 0);

	/* PWM Period 800Hz */
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 3, DUTY_100);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 3);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 enable */
	LPC_TIMER16_0->PWMC = 0x7;

	Chip_TIMER_Enable(LPC_TIMER16_0);
}

static void led_blinkcb(void)
{
	static uint8_t open = 0;

	if (open)
		led_set(LED_BLACK);
	else
		led_set(blinkcolor);
	open = ~open;
}

void led_init(void)
{
	/* System CLK 48MHz */
	Chip_TIMER_Init(LPC_TIMER16_0);
	Chip_TIMER_Disable(LPC_TIMER16_0);

	/* CT16B0_MAT0/CT16B0_MAT1/CT16B0_MAT2 init */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, PIN_LED_RED, IOCON_FUNC2 | IOCON_MODE_INACT);

	/* CT16B0_MAT0 LED_GREEN duty:50% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 0);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, DUTY_0);

	/* CT16B0_MAT1 LED_BLUE duty:25% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 1);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, DUTY_0);

	/* CT16B0_MAT2 LED_RED duty:10% */
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 2);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 2, DUTY_0);
}

void led_rgb(unsigned int rgb)
{
	timer_kill(A3233_TIMER_LED);
	led_set(rgb);
}

void led_blink(unsigned int rgb)
{
	blinkcolor = rgb;
	timer_set(A3233_TIMER_LED, 500, led_blinkcb);
}

