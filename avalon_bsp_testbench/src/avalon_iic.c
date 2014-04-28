/*
===============================================================================
 Name        : avalon_iic.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon iic api
===============================================================================
*/

/*i2c*/
#include "avalon_api.h"

#define I2C_NOP 4 //even only

void AVALON_I2c_Init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
}

void AVALON_I2c_Start(void)
{
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, FALSE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	AVALON_Delay(I2C_NOP);
}

void AVALON_I2c_Stop(void)
{
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	AVALON_Delay(I2C_NOP);
}

void AVALON_I2c_Wbyte(unsigned char data)
{
	unsigned int i;
	unsigned char data_buf = data;
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	AVALON_Delay(I2C_NOP);

	for(i=0; i<8; i++){
		if((data_buf&0x80) == 0x80)
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
		else
			Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, FALSE);
		AVALON_Delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
		AVALON_Delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
		data_buf = data_buf << 1;
		AVALON_Delay(I2C_NOP);
	}
	//master wait ACK
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, FALSE);
	AVALON_Delay(I2C_NOP);
}

unsigned char AVALON_I2c_Rbyte(void)
{
	unsigned int i;
	unsigned char data_buf = 0;

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 5);

	for(i=0; i<8; i++){
		AVALON_Delay(I2C_NOP);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
		AVALON_Delay(I2C_NOP/2);
		data_buf = data_buf << 1;
		data_buf = data_buf | (Chip_GPIO_ReadPortBit(LPC_GPIO, 0, 5)&0x1);
		AVALON_Delay(I2C_NOP/2);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
		AVALON_Delay(I2C_NOP);
	}

	//master sent ACK
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, FALSE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	AVALON_Delay(I2C_NOP);

	return data_buf;
}

