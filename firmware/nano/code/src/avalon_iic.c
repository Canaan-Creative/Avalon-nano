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
#include "avalon_iic.h"

/* PIO0_4 as scl, PIO0_5 as sda */
/* write addr: 0x92, read addr: 0x93 */
#define TMP102_ADDR_W 0x92
#define TMP102_ADDR_R 0x93

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
}

int i2c_readtemp(void)
{
	int tmp = 0;

	i2c_start();
	i2c_wbyte(TMP102_ADDR_W);
	i2c_wbyte(0x0);
	i2c_stop();

	i2c_start();
	i2c_wbyte(TMP102_ADDR_R);
	tmp = i2c_rbyte();
	tmp = tmp << 8;
	tmp = (tmp & 0xffffff00) | i2c_rbyte();
	i2c_stop();

         if (tmp > 0x7ff) {
		tmp = (~tmp + 1) & 0x7ff;
		tmp = -tmp;
	 }

	tmp >>= 4;
	return tmp;
}
