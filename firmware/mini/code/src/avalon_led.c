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

static void led_set(uint32_t rgb)
{
	uint8_t r, g, b;

	r = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	b = (rgb & 0xff);

	if (r)
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_RED, true);
	else
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_RED, false);

	if (g)
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_GREEN, true);
	else
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_GREEN, false);

	if (b)
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_BLUE, true);
	else
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_LED_BLUE, false);
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

void led_rgb(unsigned int rgb)
{
	led_set(rgb);
}

void led_blink(unsigned int rgb)
{
	/* TODO: try to use match */
}

