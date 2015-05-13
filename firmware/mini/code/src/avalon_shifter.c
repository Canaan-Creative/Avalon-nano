/*
 * @brief
 *
 * @note
 * Author: Mikeqin Fengling.Qin@gmail.com
 *
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "defines.h"
#include "libfunctions.h"
#include "avalon_shifter.h"

#define PIN_OE	14
#define PIN_DS	20
#define PIN_SHCP	17
#define PIN_STCP	16
#define PIN_PG	7
#define VOLTAGE_DELAY   100
static uint16_t g_voltage = ASIC_0V;

static void init_mux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, PIN_OE, IOCON_FUNC1);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_OE);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_DS);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_SHCP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, PIN_STCP);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, PIN_PG);
}

static void shifter_byte(uint8_t ch)
{
	uint8_t i;

	/* MSB */
	for (i = 0; i < 8; i++) {
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_SHCP, 0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_DS, (ch >> (7 - i)) & 1);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_SHCP, 1);
	}
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_SHCP, 0);
}

void shifter_init(void)
{
	init_mux();

	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_OE, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_DS, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_SHCP, 0);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_STCP, 0);
}

int set_voltage(uint16_t vol)
{
	uint8_t poweron = 0;

	if (g_voltage == vol)
		return 0;

	if (g_voltage == ASIC_0V) {
		Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_OE, 1);
		poweron = 1;
	}

	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_STCP, 0);

	shifter_byte(vol & 0xff);

	g_voltage = vol;
	if (poweron && vol != ASIC_0V)
		delay(VOLTAGE_DELAY);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_STCP, 1);
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_STCP, 0);
	__NOP();
	__NOP();
	__NOP();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, PIN_OE, 0);

	if (vol == ASIC_0V)
			return 0;

	return poweron;
}

uint16_t get_voltage(void)
{
	return g_voltage;
}

uint8_t read_power_good(void)
{
	return Chip_GPIO_ReadPortBit(LPC_GPIO, 0, PIN_PG);
}

