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
#include "avalon_timer.h"
#include "avalon_iic.h"

/* PIO0_4 as scl, PIO0_5 as sda */
/* write addr: 0x92, read addr: 0x93 */
#define TMP102_ADDR_W 0x92
#define TMP102_ADDR_R 0x93
#define TMP102_TIMER	TIMER_ID4
#define TIMER_INTERVAL	1000

static void i2c_nop(void)
{
	__NOP();
	__NOP();
}

static void i2c_start(void)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	i2c_nop();
}

static void i2c_stop()
{
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	i2c_nop();
}

static void i2c_wbyte(uint8_t data)
{
	uint8_t i;

	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	i2c_nop();

	for (i = 0; i < 8; i++) {
		if ((data & 0x80) == 0x80)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
		data <<= 1;
		i2c_nop();
	}
	//master wait ACK
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	i2c_nop();
}

static uint8_t i2c_rbyte(void)
{
	uint8_t i;
	uint8_t data_buf = 0;

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 5);

	for (i = 0; i < 8; i++) {
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
		i2c_nop();
		data_buf = data_buf << 1;
		data_buf = data_buf | (Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 5) & 0x1);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
		i2c_nop();
	}

	//master sent ACK
	i2c_nop();
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, false);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, false);
	i2c_nop();

	return data_buf;
}

void i2c_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, true);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, true);
	timer_set(TMP102_TIMER, TIMER_INTERVAL, NULL);
}

int i2c_readtemp(void)
{
	static int last_temp;
	int temp = 0;

	if (!timer_istimeout(TMP102_TIMER))
		return last_temp;

	i2c_start();
	i2c_wbyte(TMP102_ADDR_W);
	i2c_wbyte(0x0);
	i2c_stop();

	i2c_start();
	i2c_wbyte(TMP102_ADDR_R);
	temp = i2c_rbyte();
	temp = temp << 8;
	temp = (temp & 0xffffff00) | i2c_rbyte();
	i2c_stop();
	temp >>= 4;

	if (temp > 0x7ff) {
		temp = (~temp + 1) & 0x7ff;
		temp = -temp;
	}

	temp >>= 4;
	last_temp = temp;
	timer_set(TMP102_TIMER, TIMER_INTERVAL, NULL);
	return temp;
}
