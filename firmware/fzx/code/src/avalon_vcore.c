/*
 * @brief
 *
 * @note
 * Author: Fanzixiao fanzixiao@cannan-creative.com
 *
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "defines.h"
#include "libfunctions.h"
#include "avalon_vcore.h"

#define VCORE_PORT      0
#define VCORE_PIN_DS    21
#define VCORE_PIN_SHCP  7
#define VCORE_PIN_STCP  6
#define VCORE_PIN_EN1   9
#define VCORE_PIN_EN2   8

#define VOLTAGE_DELAY   40

static uint16_t voltage = ASIC_0V;

static void init_mux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_PIN_DS, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_PIN_SHCP, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_PIN_STCP, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_PIN_EN1, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_PIN_EN2, (IOCON_FUNC0 | IOCON_MODE_PULLUP));

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_PIN_DS);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_PIN_SHCP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_PIN_STCP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN1);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN2);
}

void vcore_init(void)
{
	init_mux();

	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_DS, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_SHCP, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_STCP, 0);

	vcore_disable(VCORE1);
	vcore_disable(VCORE2);
}

uint8_t set_voltage(uint16_t vol)
{
	uint8_t i;

	if (voltage == vol)
		return 0;

	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_STCP, 0);

	/* MSB first */
	for (i = 0; i < 8; i++) {
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_SHCP, 0);
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_DS, (vol >> (7 - i)) & 1);
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_SHCP, 1);
	}
	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_SHCP, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_STCP, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_STCP, 0);

	voltage = vol;

	delay(VOLTAGE_DELAY);

	return 0;
}

uint16_t get_voltage(void)
{
	return voltage;
}

void vcore_disable(uint8_t num)
{
	switch (num) {
	case 1:
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN1, 1);
		break;
	case 2:
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN2, 1);
		break;
	default:
		break;
	}
}

void vcore_enable(uint8_t num)
{
	switch (num) {
	case 1:
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN1, 0);
		break;
	case 2:
		Chip_GPIO_SetPinState(LPC_GPIO, VCORE_PORT, VCORE_PIN_EN2, 0);
		break;
	default:
		break;
	}
}
