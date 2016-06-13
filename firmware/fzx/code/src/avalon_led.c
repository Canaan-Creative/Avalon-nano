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

#define LED_POINT     0
#define LED_CLK_PIN   11
#define LED_DATA_PIN  9

#define LED_CLK_H     1
#define LED_CLK_L     0

#define LED_ON        1
#define LED_OFF       0

static uint8_t led_state = 0;

void test_io_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 7, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 2);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 1);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, 0);
}

void led_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_POINT, LED_CLK_PIN, (IOCON_FUNC1 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, LED_POINT, LED_DATA_PIN, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_POINT, LED_CLK_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, LED_POINT, LED_DATA_PIN);

	Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_CLK_PIN, LED_CLK_L);
}


void led_set(uint8_t led_data)
{
	uint8_t i;

	if (led_data)
		led_state |= led_data; 
	else
		led_state &= ~led_data;

	Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_CLK_PIN, LED_CLK_L);
	for (i = 0; i < 8; i++) {
		if (led_data & 0x80)
			Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_DATA_PIN, LED_ON);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_DATA_PIN, LED_OFF);

		asm volatile("NOP");
		Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_CLK_PIN, LED_CLK_H);
		led_data <<= 1;
		asm volatile("NOP");
		Chip_GPIO_SetPinState(LPC_GPIO, LED_POINT, LED_CLK_PIN, LED_CLK_L);
	}
}
