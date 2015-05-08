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

/* V_25 USB, V_18 A3233 IO Power, V_CORE A3233 Core, V_09 A3233 Pll*/
#define ADC_CHANNEL_V_25	(ADC_CH1)
#define ADC_CHANNEL_V_18	(ADC_CH3)
#define ADC_CHANNEL_V_CORE	(ADC_CH5)
#define ADC_CHANNEL_V_09	(ADC_CH7)

void adc_init(void);
void adc_read(uint8_t channel, uint16_t *data);

#endif /* __AVALON_ADC_H_ */
