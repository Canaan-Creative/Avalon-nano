/*
 ===============================================================================
 Name        : avalon_iic.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon iic api
 Note: Chip IIC cann't work sometimes,It may be conflict with the usb.
 ===============================================================================
 */

#include "chip.h"
#include "avalon_api.h"

//write:0x92, read:0x93
#define I2C_NOP 4 //even only
#define I2C_ADDR_W 0x92
#define I2C_ADDR_R 0x93

static void I2c_Start(){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, FALSE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, FALSE);
	AVALON_Delay(I2C_NOP);
}

static void I2c_Stop(){
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	AVALON_Delay(I2C_NOP);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
	AVALON_Delay(I2C_NOP);
}

static void I2c_w_byte(unsigned char data){
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

unsigned char I2c_r_byte(){
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

void AVALON_I2C_Init(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
}

unsigned int AVALON_I2C_TemperRd(void)
{
	unsigned int tmp = 0;
	I2c_Start();
	I2c_w_byte(I2C_ADDR_W);
	I2c_w_byte(0x0);//temperature register
	I2c_Stop();
	I2c_Start();
	I2c_w_byte(I2C_ADDR_R);
	tmp = I2c_r_byte()&0xff;
	tmp = tmp << 8;
	tmp = (tmp&0xffffff00) | I2c_r_byte();
	I2c_Stop();
	tmp = (((tmp >> 4)&0xfff)/4)*0.25;
	return tmp;
}
