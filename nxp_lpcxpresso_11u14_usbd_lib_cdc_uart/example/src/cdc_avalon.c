/*
 * @brief avalon routines
 *
 * @note
 * Copyright(C) 0xf8, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include "cdc_avalon.h"

static Bool		a3233_poweren = FALSE;

void AVALON_POWER_Enable(Bool On)
{
	a3233_poweren = On;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, On);//VCore Enable
}

Bool AVALON_POWER_IsEnable(void)
{
	return a3233_poweren;
}

static void Init_POWER()
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);//VID0
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);//VID1
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);//VCore Enable
	*(unsigned int *) 0x4004402c = 0x81;

	AVALON_POWER_Enable(FALSE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, TRUE);//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, TRUE);//VID1
}

#define VCORE_0P9   0x0
#define VCORE_0P8   0x1
#define VCORE_0P725 0x2
#define VCORE_0P675 0x3

static void POWER_Cfg(unsigned char VID){
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, (bool)(VID&1));//VID0
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, (bool)(VID>>1));//VID1
}

static void Init_Rstn()
{
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, TRUE);
}

void AVALON_Delay(unsigned int max)
{
	volatile unsigned int i;
	for(i = 0; i < max; i++);
}

void AVALON_Rstn_A3233()
{
	AVALON_Delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, TRUE);
	AVALON_Delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, FALSE);
	AVALON_Delay(2000);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, TRUE);
	AVALON_Delay(2000);
}

/*i2c*/
#define I2C_NOP 4 //even only
#define I2C_ADDR_W 0x92 //write:0x92, read:0x93
#define I2C_ADDR_R 0x93 //write:0x92, read:0x93
static void Init_I2c(){
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_STDI2C_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);

	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, TRUE);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, TRUE);
}

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

unsigned int tmp102_rd(){
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

/*ACD*/
static void Init_ADC_PinMux(void)
{
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, FUNC1);
}

static void ADC_Rd(uint8_t channel, uint16_t *data){
	Chip_ADC_EnableChannel(LPC_ADC, channel, ENABLE);
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, channel, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, channel, data);
	Chip_ADC_EnableChannel(LPC_ADC, channel, DISABLE);
}

/* V_25 USB, V_18 A3233 IO Power, V_CORE A3233 Core, V_09 A3233 Pll*/
#define V_25	0
#define V_18	1
#define V_CORE	2
#define V_09	3
static float ADC_Guard(int type)
{
	uint16_t dataADC;
	float vol;
	switch (type) {
	case V_25:
		ADC_Rd(ADC_CH1, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_18:
		ADC_Rd(ADC_CH3, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_CORE:
		ADC_Rd(ADC_CH5, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	case V_09:
		ADC_Rd(ADC_CH7, &dataADC);
		vol = (dataADC/1024) * 3.3;
		break;
	}

	return vol;
}

#ifdef HIGH_BAND
#define FREF_MIN 25
#define FREF_MAX 50
#define FVCO_MIN 1500
#define FVCO_MAX 3000
#define FOUT_MIN 187.5
#define FOUT_MAX 3000
#define WORD0_BASE 0x80000007
#else
#define FREF_MIN 10
#define FREF_MAX 50
#define FVCO_MIN 800
#define FVCO_MAX 1600
#define FOUT_MIN 100
#define FOUT_MAX 1600
#define WORD0_BASE 0x7
#endif

/*
 * @brief	gen pll cfg val (freq 100-450 )
 * @return	pll cfg val
 * */
unsigned int AVALON_Gen_A3233_Pll_Cfg(unsigned int freq, unsigned int *actfreq){
	unsigned int NOx[4] , i=0;
	unsigned int NO =0;//1 2 4 8
	unsigned int Fin = 25;
	unsigned int NR =0;
	unsigned int Fvco =0;
	unsigned int Fout = 200 ;
	unsigned int NF =0;
	unsigned int Fref =0;
	unsigned int OD ;
	unsigned int tmp ;
	NOx[0] = 1 ;
	NOx[1] = 2 ;
	NOx[2] = 4 ;
	NOx[3] = 8 ;

	for(Fout = freq ; Fout <= 1300 ; Fout=Fout+1)
	for(i=0;i<4;i++){
		NO = NOx[i] ;
		for(NR=1;NR<32;NR++)
			for(NF=1;NF<128;NF++){
				Fref = Fin/NR ;
				Fvco = Fout*NO ;
				if(
				((Fout) == ((Fin*NF/(NR*NO))) ) &&
				(FREF_MIN<=Fref&&Fref<=FREF_MAX) &&
				(FVCO_MIN<=Fvco&&Fvco<=FVCO_MAX) &&
				(FOUT_MIN<=Fout&&Fout<=FOUT_MAX)
				){
					if(NO == 1) OD = 0 ;
					if(NO == 2) OD = 1 ;
					if(NO == 4) OD = 2 ;
					if(NO == 8) OD = 3 ;

					tmp =   WORD0_BASE     |
						((NR-1)&0x1f)<<16  |
						((NF/2-1)&0x7f)<<21|
						(OD<<28) ;

					if(actfreq)
						*actfreq = Fout;
					return tmp;
				}
		}
	}

	if(actfreq)
		*actfreq = 0;
	return 0;
}

void AVALON_led_rgb(unsigned int rgb)
{
	if (rgb == AVALON_LED_GREEN) {
		Board_LED_Set(0, FALSE);//green
		Board_LED_Set(1, TRUE);//red
		Board_LED_Set(2, TRUE);//blue
	} else if (rgb == AVALON_LED_RED) {
		Board_LED_Set(0, TRUE);//green
		Board_LED_Set(1, FALSE);//red
		Board_LED_Set(2, TRUE);//blue
	} else if (rgb == AVALON_LED_BLUE) {
		Board_LED_Set(0, TRUE);//green
		Board_LED_Set(1, TRUE);//red
		Board_LED_Set(2, FALSE);//blue
	} else {
		Board_LED_Set(0, TRUE);//green
		Board_LED_Set(1, TRUE);//red
		Board_LED_Set(2, TRUE);//blue
	}
}

void Init_Pwm()
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT16B0);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, IOCON_FUNC2 | IOCON_MODE_INACT);//red
	*(unsigned int *)0x4000c004 = 0x1;//enable tcr, ct16b0
	*(unsigned int *)0x4000c008 = 0xffff;//timer counter
	*(unsigned int *)0x4000c03c = 0x1 | (0x3<<8);//External Match
	*(unsigned int *)0x4000c074 = 0x4;//pwm
	*(unsigned int *)0x4000c020 = 0xffff/4;
}

void Init_Gpio(){
	Chip_GPIO_Init(LPC_GPIO);
}

static void Init_Led(void)
{
	/* Set the PIO_7 as output */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 17);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 15);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 19);
}

static void Init_CLKOUT_PinMux(void)
{
#if (defined(BOARD_NXP_XPRESSO_11U14) || defined(BOARD_NGX_BLUEBOARD_11U24))
	/* LPC11U14 Xpresso board has CLKOUT on pin PIO0_1 on J6-38 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#elif defined(BOARD_NXP_XPRESSO_11C24)
	/* LPC11C24 Xpresso board has CLKOUT on pin PIO0_1 on J6-38 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO0_1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#elif defined(BOARD_MCORE48_1125)
	/* LPC1125 MCore48 board has CLKOUT on pin PIO0_1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO0_1, (IOCON_FUNC1 | IOCON_MODE_INACT));
#else
	#error "Pin MUX for CLKOUT not configured"
#endif
}

static void CLKOUT_Cfg(bool On)
{
	if(On == TRUE)
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);
	else
		Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 0);
}

void Init_Counter(){
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CT32B0);
}

ErrorCode_t AVALON_init (void){
	ErrorCode_t ret = LPC_OK;
	ADC_CLOCK_SETUP_T ADCSetup;

	Init_Gpio();
	Init_Led();
	Init_Rstn();//low active
	Init_CLKOUT_PinMux();
	Init_POWER();
	Init_I2c();
	Init_ADC_PinMux();
	Chip_ADC_Init(LPC_ADC, &ADCSetup);

	POWER_Cfg(VCORE_0P675);
	CLKOUT_Cfg(TRUE);
	AVALON_POWER_Enable(FALSE);

	AVALON_led_rgb(AVALON_LED_OFF);

	return ret;
}
