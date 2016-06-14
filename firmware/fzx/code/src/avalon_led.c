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

#define LED_12V_1T_PORT  0
#define LED_12V_1F_PORT  0
#define LED_12V_2T_PORT  0
#define LED_12V_2F_PORT  0

#define LED_12V_1T_PIN   0
#define LED_12V_1F_PIN   1
#define LED_12V_2T_PIN   4
#define LED_12V_2F_PIN   5

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
