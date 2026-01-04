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

extern volatile uint16_t adc_dma_buf[ADC_DMA_BUF_LEN];
extern volatile uint16_t adc_pc4[ADC_DMA_BUF_LEN / 2];
extern volatile uint16_t adc_pc5[ADC_DMA_BUF_LEN / 2];

uint32_t BKEL_BSW_ADC_GetValue(ADC_HandleTypeDef* hadc);


#endif /* INC_BKEL_BSW_ADC_H_ */
