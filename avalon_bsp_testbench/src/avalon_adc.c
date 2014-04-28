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
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, FUNC1);
}

void AVALON_ADC_Rd(uint8_t channel, uint16_t *data)
{
	Chip_ADC_EnableChannel(LPC_ADC, channel, ENABLE);
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, channel, ADC_DR_DONE_STAT) != SET) {}
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, channel, data);
	Chip_ADC_EnableChannel(LPC_ADC, channel, DISABLE);
}
