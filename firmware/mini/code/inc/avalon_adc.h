/*
 * @brief adc head file
 *
 * @note
 *
 * @par
 */
#ifndef __AVALON_ADC_H_
#define __AVALON_ADC_H_

#include "adc_11xx.h"

#define ADC_CHANNEL_12V	(ADC_CH1)
#define ADC_CHANNEL_COPPER	(ADC_CH2)
#define ADC_CHANNEL_FAN	(ADC_CH7)

void adc_init(void);
void adc_read(uint8_t channel, uint16_t *data);

#endif /* __AVALON_ADC_H_ */
