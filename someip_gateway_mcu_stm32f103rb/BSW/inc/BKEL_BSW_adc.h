/*
 * BKEL_adc.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_BSW_ADC_H_
#define INC_BKEL_BSW_ADC_H_

#include "stm32f1xx_hal.h"

#define ADC_DMA_BUF_LEN  16U
#define ADC_CH_COUNT	 2U			// 채널2개 ch14, ch15
#define ADC_INVALID_VALUE	(UINT32_MAX)
extern volatile uint16_t adc_dma_buf[ADC_DMA_BUF_LEN];
extern volatile uint16_t adc_pc4[ADC_DMA_BUF_LEN / 2];
extern volatile uint16_t adc_pc5[ADC_DMA_BUF_LEN / 2];

uint32_t BKEL_BSW_ADC_GetValue(void);
void getValueTest(void);

#endif /* INC_BKEL_BSW_ADC_H_ */
