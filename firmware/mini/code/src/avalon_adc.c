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
#include "avalon_adc.h"

void adc_init(void)
{
	ADC_CLOCK_SETUP_T ADCSetup;

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, FUNC1);
	Chip_ADC_Init(LPC_ADC, &ADCSetup);
}

void adc_read(uint8_t channel, uint16_t *data)
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

