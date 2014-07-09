/*
===============================================================================
 Name        : avalon_adc.c
 Author      : Mikeqin
 Version     : 0.1
 Copyright   : GPL
 Description : avalon adc api
===============================================================================
*/
#include "avalon_api.h"

void AVALON_ADC_Init(void)
{
	ADC_CLOCK_SETUP_T ADCSetup;
	/*
	 * PIO0_12 AD1 V_25
	 * PIO0_14 AD3 V_18
	 * PIO0_16 AD5 V_CORE
	 * PIO0_23 AD7 V_09
	 * */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, FUNC1);
	Chip_ADC_Init(LPC_ADC, &ADCSetup);
}

void AVALON_ADC_Rd(uint8_t channel, uint16_t *data)
{
	Chip_ADC_EnableChannel(LPC_ADC, (ADC_CHANNEL_T)channel, ENABLE);
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, (ADC_CHANNEL_T)channel, ADC_DR_DONE_STAT) != SET);
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, channel, data);
	Chip_ADC_EnableChannel(LPC_ADC, (ADC_CHANNEL_T)channel, DISABLE);
}
