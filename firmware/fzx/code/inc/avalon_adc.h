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

#define ADC_PORT      0
#define ADC_VCORE_PIN 13
#define ADC_V12V_PIN  12

#define ADC_CHANNEL_V12V  (ADC_CH1)
#define ADC_CHANNEL_VCORE (ADC_CH2)

void adc_init(void);
void adc_read(uint8_t channel, uint16_t *data);

#endif /* __AVALON_ADC_H_ */
