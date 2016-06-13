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
/* write addr: 0x90, read addr: 0x91 */
#define TMP102_ADDR_W	0x90
#define TMP102_ADDR_R	0x91
#define TMP102_TIMER	TIMER_ID4
#define TIMER_INTERVAL	1000

static void i2c_nop(void)
{
	__NOP();
	__NOP();
	__NOP();
	__NOP();
}

static void i2c_start(void)
{
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_L);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
	i2c_nop();
}

static void i2c_stop()
{
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
	i2c_nop();
}

static void i2c_wbyte(uint8_t data)
{
	uint8_t i;

	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
	i2c_nop();

	for (i = 0; i < 8; i++) {
		if ((data & 0x80) == 0x80)
			Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_L);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
		data <<= 1;
		i2c_nop();
	}
	//master wait ACK
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_L);
	i2c_nop();
}

static uint8_t i2c_rbyte(void)
{
	uint8_t i;
	uint8_t data_buf = 0;

	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, I2C_PORT, I2C_SDA_PIN);

	for (i = 0; i < 8; i++) {
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
		i2c_nop();
		data_buf = data_buf << 1;
		data_buf = data_buf | (Chip_GPIO_ReadPortBit(LPC_GPIO, I2C_PORT, I2C_SDA_PIN) & 0x1);
		i2c_nop();
		Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
		i2c_nop();
	}

	//master sent ACK
	i2c_nop();
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, I2C_PORT, I2C_SDA_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_L);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
	i2c_nop();
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_L);
	i2c_nop();

	return data_buf;
}

void i2c_init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_PORT, I2C_SCL_PIN, IOCON_FUNC0 | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_PORT, I2C_SDA_PIN, IOCON_FUNC0 | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, I2C_PORT, I2C_SCL_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, I2C_PORT, I2C_SDA_PIN);

	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SCL_PIN, I2C_SCL_H);
	Chip_GPIO_SetPinState(LPC_GPIO, I2C_PORT, I2C_SDA_PIN, I2C_SDA_H);
	timer_set(TMP102_TIMER, TIMER_INTERVAL);
}

int i2c_readtemp(void)
{
	static int last_temp = 100;
	int temp = 0;

	if (!timer_istimeout(TMP102_TIMER))
		return last_temp;

	i2c_start();
	i2c_wbyte(TMP102_ADDR_W);
	i2c_wbyte(0x0);
	i2c_stop();

	i2c_start();
	i2c_wbyte(TMP102_ADDR_R);
	temp = i2c_rbyte() & 0xff;
	temp = temp << 8;
	temp = (temp & 0xffffff00) | (i2c_rbyte() & 0xff);
	i2c_stop();
	temp >>= 4;

	if (temp > 0x7ff) {
		temp = (~temp + 1) & 0x7ff;
		temp = -temp;
	}

	temp >>= 4;
	last_temp = temp;
	timer_set(TMP102_TIMER, TIMER_INTERVAL);
	return temp;
}
