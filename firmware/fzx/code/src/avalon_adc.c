/*
 * @brief
 *
 * @note
 * Author: Fanzixiao fanzixiao@cannan-creative.com
 *
 * @par
 * This is free and unencumbered software released into the public domain.
 * For details see the UNLICENSE file at the root of the source tree.
 */
#include "board.h"
#include "avalon_adc.h"

#define ADC_VBASE_PORT    0
#define ADC_VBASE_PIN     23
#define ADC_NTC1_PORT     0
#define ADC_NTC1_PIN      14
#define ADC_NTC2_PORT     0
#define ADC_NTC2_PIN      22
#define ADC_V12V_1_PORT   0
#define ADC_V12V_1_PIN    16
#define ADC_V12V_2_PORT   0
#define ADC_V12V_2_PIN    11
#define ADC_VCORE1_PORT   0
#define ADC_VCORE1_PIN    13
#define ADC_VCORE2_PORT   0
#define ADC_VCORE2_PIN    12

void adc_init(void)
{
	ADC_CLOCK_SETUP_T ADCSetup;

	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_VBASE_PORT, ADC_VBASE_PIN, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_NTC1_PORT, ADC_NTC1_PIN, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_NTC2_PORT, ADC_NTC2_PIN, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_V12V_1_PORT, ADC_V12V_1_PIN, FUNC1);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_V12V_2_PORT, ADC_V12V_2_PIN, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_VCORE1_PORT, ADC_VCORE1_PIN, FUNC2);
	Chip_IOCON_PinMuxSet(LPC_IOCON, ADC_VCORE2_PORT, ADC_VCORE2_PIN, FUNC2);
	Chip_ADC_Init(LPC_ADC, &ADCSetup);
}

void adc_read(uint8_t channel, uint16_t *data)
{
	Chip_ADC_EnableChannel(LPC_ADC, (ADC_CHANNEL_T)channel, ENABLE);
	/* Start A/D conversion */
	Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);
	/* Waiting for A/D conversion complete */
	while (Chip_ADC_ReadStatus(LPC_ADC, (ADC_CHANNEL_T)channel, ADC_DR_DONE_STAT) != SET)
		;
	/* Read ADC value */
	Chip_ADC_ReadValue(LPC_ADC, channel, data);
	Chip_ADC_EnableChannel(LPC_ADC, (ADC_CHANNEL_T)channel, DISABLE);
}
