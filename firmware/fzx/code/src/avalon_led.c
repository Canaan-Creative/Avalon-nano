/*
 * @brief
 *
 * @note
 * Author: fanzixiao@canaan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "avalon_led.h"
#include "avalon_timer.h"

#define LED_12V_1T_PORT  0
#define LED_12V_1F_PORT  0
#define LED_12V_2T_PORT  0
#define LED_12V_2F_PORT  0

#define LED_12V_1T_PIN   0
#define LED_12V_1F_PIN   1
#define LED_12V_2T_PIN   4
#define LED_12V_2F_PIN   5

static unsigned int led_blink_flag = 0;

void led_set(unsigned int led, unsigned int state)
{
	switch (led) {
	case LED_12V_1T:
		if (state == LED_ON)
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_1T_PORT, LED_12V_1T_PIN, LED_ON);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_1T_PORT, LED_12V_1T_PIN, LED_OFF);
		break;
	case LED_12V_1F:
		if (state == LED_ON)
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_1F_PORT, LED_12V_1F_PIN, LED_ON);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_1F_PORT, LED_12V_1F_PIN, LED_OFF);
		break;
	case LED_12V_2T:
		if (state == LED_OFF)
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_2T_PORT, LED_12V_2T_PIN, LED_ON);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_2T_PORT, LED_12V_2T_PIN, LED_OFF);
		break;
	case LED_12V_2F:
		if (state == LED_OFF)
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_2F_PORT, LED_12V_2F_PIN, LED_ON);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, LED_12V_2F_PORT, LED_12V_2F_PIN, LED_OFF);
		break;
	default:
		break;
	}
}

static void led_blink_12v(void)
{
	static uint8_t open_12v = 0;

	if (open_12v) {
		led_set(LED_12V_1T, LED_ON);
		led_set(LED_12V_1F, LED_OFF);
		led_set(LED_12V_2T, LED_ON);
		led_set(LED_12V_2F, LED_OFF);
	} else {
		led_set(LED_12V_1T, LED_OFF);
		led_set(LED_12V_1F, LED_ON);
		led_set(LED_12V_1T, LED_OFF);
		led_set(LED_12V_1F, LED_ON);
	}
	open_12v = ~open_12v;
}

static void led_blink_12v_1t(void)
{
	static uint8_t open_12v_1t = 0;

	if (open_12v_1t)
		led_set(LED_12V_1T, LED_ON);
	else
		led_set(LED_12V_1T, LED_OFF);
	open_12v_1t = ~open_12v_1t;
}

static void led_blink_12v_1f(void)
{
	static uint8_t open_12v_1f = 0;

	if (open_12v_1f)
		led_set(LED_12V_1F, LED_ON);
	else
		led_set(LED_12V_1F, LED_OFF);
	open_12v_1f = ~open_12v_1f;
}

static void led_blink_12v_2t(void)
{
	static uint8_t open_12v_2t = 0;

	if (open_12v_2t)
		led_set(LED_12V_2T, LED_ON);
	else
		led_set(LED_12V_2T, LED_OFF);
	open_12v_2t = ~open_12v_2t;
}

static void led_blink_12v_2f(void)
{
	static uint8_t open_12v_2f = 0;

	if (open_12v_2f)
		led_set(LED_12V_2F, LED_ON);
	else
		led_set(LED_12V_2F, LED_OFF);
	open_12v_2f = ~open_12v_2f;
}

void led_rgb(unsigned int rgb, unsigned int state)
{
	if (led_blink_flag & rgb)
		return ;

	led_set(rgb, state);
}

void led_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_12V_1T_PORT, LED_12V_1T_PIN, (IOCON_FUNC1 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_12V_1F_PORT, LED_12V_1F_PIN, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_12V_2T_PORT, LED_12V_2T_PIN, (IOCON_FUNC0 | IOCON_STDI2C_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_12V_2F_PORT, LED_12V_2F_PIN, (IOCON_FUNC0 | IOCON_STDI2C_EN));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_12V_1T_PORT, LED_12V_1T_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_12V_1F_PORT, LED_12V_1F_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_12V_2T_PORT, LED_12V_2T_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_12V_2F_PORT, LED_12V_2F_PIN);

	led_set(LED_12V_1T, LED_OFF);
	led_set(LED_12V_1F, LED_OFF);
	led_set(LED_12V_2T, LED_OFF);
	led_set(LED_12V_2F, LED_OFF);
}

void led_blink_on(unsigned int led)
{
	switch (led) {
	case LED_12V:
		timer_set(TIMER_LED_12V, 500, led_blink_12v);
		break;
	case LED_12V_1T:
		led_blink_flag |= LED_12V_1T;
		timer_set(TIMER_LED_12V_1T, 500, led_blink_12v_1t);
		break;
	case LED_12V_1F:
		led_blink_flag |= LED_12V_1F;
		timer_set(TIMER_LED_12V_1F, 500, led_blink_12v_1f);
		break;
	case LED_12V_2T:
		led_blink_flag |= LED_12V_2T;
		timer_set(TIMER_LED_12V_2T, 500, led_blink_12v_2t);
		break;
	case LED_12V_2F:
		led_blink_flag |= LED_12V_2F;
		timer_set(TIMER_LED_12V_2F, 500, led_blink_12v_2f);
		break;
	default:
		break;
	}
}

void led_blink_off(unsigned int led)
{
	switch (led) {
	case LED_12V:
		timer_kill(TIMER_LED_12V);
		break;
	case LED_12V_1T:
		led_blink_flag &= ~LED_12V_1T;
		timer_kill(TIMER_LED_12V_1T);
		break;
	case LED_12V_1F:
		led_blink_flag &= ~LED_12V_1F;
		timer_kill(TIMER_LED_12V_1F);
		break;
	case LED_12V_2T:
		led_blink_flag &= ~LED_12V_2T;
		timer_kill(TIMER_LED_12V_2T);
		break;
	case LED_12V_2F:
		led_blink_flag &= ~LED_12V_2F;
		timer_kill(TIMER_LED_12V_2F);
		break;
	default:
		break;
	}
}
