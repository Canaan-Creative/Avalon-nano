/*
 ===============================================================================
 Name        : avalon_iic.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon iic api
 ===============================================================================
 */

#include "chip.h"
#include "avalon_api.h"

//write:0x92, read:0x93
#define I2C_ADDR_W 0x92
#define I2C_ADDR_R 0x93

/**
 * @brief	I2C Interrupt Handler
 * @return	None
 */
void I2C_IRQHandler(void)
{
	if (Chip_I2C_IsMasterActive(I2C0)) {
		Chip_I2C_MasterStateHandler(I2C0);
	}
	else {
		Chip_I2C_SlaveStateHandler(I2C0);
	}
}

void AVALON_I2c_Init(void)
{
	Chip_SYSCTL_PeriphReset(RESET_I2C0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC1 | IOCON_SFI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC1 | IOCON_SFI2C_EN);

	/* Initialize I2C, CLK 100kHz */
	Chip_I2C_Init(I2C0);
	Chip_I2C_SetClockRate(I2C0, 100000);

	NVIC_EnableIRQ(I2C0_IRQn);
}

unsigned int AVALON_I2c_TemperRd()
{
	unsigned int 	tmp = 0;
	uint8_t			tempreg = 0;
	uint8_t			tempval[2];

	Chip_I2C_MasterSend(I2C0, I2C_ADDR_W>>1, &tempreg, 1);
	Chip_I2C_MasterRead(I2C0, I2C_ADDR_R>>1, tempval, 2);

	tmp = tempval[0]&0xff;
	tmp = tmp << 8;
	tmp = (tmp&0xffffff00) | tempval[1];
	tmp = (((tmp >> 4)&0xfff)/4)*0.25;

	return tmp;
}
