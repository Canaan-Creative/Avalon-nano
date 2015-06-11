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

#define DUTY_100	(uint32_t)(255)

#define PIN_LED_BLUE	8
#define PIN_LED_GREEN	9
#define PIN_LED_RED	11

static volatile uint8_t breath[2]; /* Only support GREEN and BLUE */

static void led_rgb(uint32_t led, uint8_t led_op)
{
	if (led == LED_RED)
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_RED, (led_op == LED_ON) ? true : false);

	if (led == LED_GREEN) {
		breath[0] = 0;
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_GREEN, (led_op == LED_ON) ? true : false);
	}

	if (led == LED_BLUE) {
		breath[1] = 0;
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_BLUE, (led_op == LED_ON) ? true : false);
	}
}

/*
 * http://sean.voisen.org/blog/2011/10/breathing-led-with-arduino
 * exp(sin(x * 2 / 3.14)) - 0.37) * 108.49
 * */
static uint8_t led_pwmcalc(void)
{
	static uint8_t pwmticks;
	uint8_t pwm_dat[] = {
		0, 4, 29, 88, 183, 252, 219, 123, 47, 10
	};

	return pwm_dat[pwmticks++ % 10];
}

static void led_breath(uint32_t led)
{
	if (led == LED_GREEN) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC2 | IOCON_MODE_INACT);
		breath[0] = 1;
	}

	if (led == LED_BLUE) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC2 | IOCON_MODE_INACT);
		breath[1] = 1;
	}
}

void TIMER16_0_IRQHandler(void)
{
	static uint32_t timer_cnt;

	Chip_TIMER_ClearMatch(LPC_TIMER16_0, 3);

	if ((breath[0] || breath[1]) && !timer_cnt) {
		uint8_t pwm = led_pwmcalc();

		/* GREEN */
		if (breath[0]) {
			Chip_TIMER_ClearMatch(LPC_TIMER16_0, 1);
			Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 1);
			Chip_TIMER_SetMatch(LPC_TIMER16_0, 1, pwm);
		}
		/* BLUE */
		if (breath[1]) {
			Chip_TIMER_ClearMatch(LPC_TIMER16_0, 0);
			Chip_TIMER_ExtMatchControlSet(LPC_TIMER16_0, 1, TIMER_EXTMATCH_SET, 0);
			Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, pwm);
		}
	}

	/* color per second = 40 / 188 (~0.21s) */
	if ((breath[0] || breath[1]) && (timer_cnt++ == 40))
		timer_cnt = 0;
}

void led_init(void)
{
	breath[0] = breath[1] = 0;

	/* System CLK 48MHz */
	Chip_TIMER_Init(LPC_TIMER16_0);
	Chip_TIMER_Disable(LPC_TIMER16_0);

	/* CT16B0_MAT1/CT16B0_MAT0/CT32B0_MAT3 init */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_RED, IOCON_FUNC1 | IOCON_DIGMODE_EN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_GREEN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_BLUE);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_RED);

	/* PWM Period ~188Hz */
	Chip_TIMER_PrescaleSet(LPC_TIMER16_0, 999);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 3, DUTY_100);
	Chip_TIMER_MatchEnableInt(LPC_TIMER16_0, 3);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 3);

	/* CT16B0_MAT0/CT16B0_MAT1 Enable */
	LPC_TIMER16_0->PWMC = 0x3;

	NVIC_ClearPendingIRQ(TIMER_16_0_IRQn);
	NVIC_EnableIRQ(TIMER_16_0_IRQn);
	Chip_TIMER_Enable(LPC_TIMER16_0);
}

void led_set(unsigned int led, uint8_t led_op)
{
	if (led_op == LED_BREATH)
		led_breath(led);
	else
		led_rgb(led, led_op);
}

