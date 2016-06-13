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

#define VCORE_PORT       0
#define VCORE_BIT1       7
#define VCORE_BIT2       8
#define VCORE_BIT3       17
#define VCORE_BIT4       20
#define VCORE_BIT5       23

#define VCORE_SHIFT_BIT1 7
#define VCORE_SHIFT_BIT2 7
#define VCORE_SHIFT_BIT3 15
#define VCORE_SHIFT_BIT4 17
#define VCORE_SHIFT_BIT5 19

#define VOLTAGE_DELAY    40

static uint16_t g_voltage = ASIC_0V;

void vcore_init(void)
{
	uint32_t vcore_value;
	uint32_t vcore_vmask;

	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_BIT1, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_BIT2, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_BIT3, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_BIT4, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_PinMuxSet(LPC_IOCON, VCORE_PORT, VCORE_BIT5, IOCON_FUNC0 | IOCON_MODE_INACT);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_BIT1);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_BIT2);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_BIT3);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_BIT4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, VCORE_PORT, VCORE_BIT5);
	
	vcore_vmask = ~((1 << VCORE_BIT1) | (1 << VCORE_BIT2) | (1 << VCORE_BIT3)\
		       |(1 << VCORE_BIT4) | (1 << VCORE_BIT5));
	Chip_GPIO_SetPortMask(LPC_GPIO, VCORE_PORT, vcore_vmask);
	
	vcore_value = ((g_voltage & 0x01) << VCORE_SHIFT_BIT1) | ((g_voltage & 0x02) << VCORE_SHIFT_BIT2) \
		     |((g_voltage & 0x04) << VCORE_SHIFT_BIT3) | ((g_voltage & 0x08) << VCORE_SHIFT_BIT4) \
		     |((g_voltage & 0x10) << VCORE_SHIFT_BIT5);
	Chip_GPIO_SetMaskedPortValue(LPC_GPIO, VCORE_PORT, vcore_value);
}

uint8_t set_voltage(uint16_t vol)
{
	uint8_t  poweron = 0;
	uint32_t vcore_value;

	if (g_voltage == vol)
		return 0;

	if (g_voltage == ASIC_0V) {
		Chip_GPIO_SetMaskedPortValue(LPC_GPIO, VCORE_PORT, 0);
		poweron = 1;
	}

	vcore_value = ((vol & 0x01) << VCORE_SHIFT_BIT1) | ((vol & 0x02) << VCORE_SHIFT_BIT2) \
		     |((vol & 0x04) << VCORE_SHIFT_BIT3) | ((vol & 0x08) << VCORE_SHIFT_BIT4) \
		     |((vol & 0x10) << VCORE_SHIFT_BIT5);
	
	Chip_GPIO_SetMaskedPortValue(LPC_GPIO, VCORE_PORT, vcore_value);

	g_voltage = vol;
	if (poweron && vol != ASIC_0V)
		delay(VOLTAGE_DELAY);

	if (vol == ASIC_0V)
		return 0;

	return poweron;
}

uint16_t get_voltage(void)
{
	return g_voltage;
}
