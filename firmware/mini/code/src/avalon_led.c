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

#define PIN_LED_BLUE	8
#define PIN_LED_GREEN	9
#define PIN_LED_RED	11

static void led_rgb(uint32_t led, uint8_t led_op)
{
	if (led == LED_RED) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_RED, IOCON_FUNC1 | IOCON_DIGMODE_EN);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_RED, (led_op == LED_ON) ? true : false);
	}

	if (led == LED_GREEN) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_GREEN, (led_op == LED_ON) ? true : false);
	}

	if (led == LED_BLUE) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_BLUE, (led_op == LED_ON) ? true : false);
	}
}

static void led_breath(uint32_t led)
{
	/* TODO */
}

void led_init(void)
{
	/* CT16B0_MAT1/CT16B0_MAT0/CT32B0_MAT3 init */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_GREEN, IOCON_FUNC0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_BLUE, IOCON_FUNC0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_LED_RED, IOCON_FUNC1 | IOCON_DIGMODE_EN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_GREEN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_BLUE);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_LED_RED);
}

void led_set(unsigned int led, uint8_t led_op)
{
	if (led_op == LED_BREATH)
		led_breath(led);
	else
		led_rgb(led, led_op);
}

