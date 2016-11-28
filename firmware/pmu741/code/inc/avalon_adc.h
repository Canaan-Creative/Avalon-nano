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

#define ADC_CHANNEL_VBASE   (ADC_CH7)
#define ADC_CHANNEL_NTC1    (ADC_CH3)
#define ADC_CHANNEL_NTC2    (ADC_CH6)
#define ADC_CHANNEL_V12V_1  (ADC_CH5)
#define ADC_CHANNEL_V12V_2  (ADC_CH5)
#define ADC_CHANNEL_VCORE1  (ADC_CH1)
#define ADC_CHANNEL_VCORE2  (ADC_CH2)

#define ADC_CAPCOUNT        7
#define ADC_CHECK_COUNT     30
#define ADC_DATA_LEN        3
#define ADC_VBASE_STD_VALUE 775.76f /* (2^10)/3.3 * 2.5 */
#define ADC_WAVE_VALUE      120 /* 0.38v = 120 / 1024 * 3.3 */

void adc_init(void);
void adc_read(uint8_t channel, uint16_t *data);
float adc_check(void);

#endif /* __AVALON_ADC_H_ */
