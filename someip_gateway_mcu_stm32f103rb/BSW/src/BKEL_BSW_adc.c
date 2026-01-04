/*
 * BKEL_adc.c
 *
 *  Created on: Dec 20, 2025
 *      Author: PNYcom
 */

#include <BKEL_BSW_adc.h>

volatile uint16_t adc_pc4[ADC_DMA_BUF_LEN / 2];
volatile uint16_t adc_pc5[ADC_DMA_BUF_LEN / 2];

uint32_t BKEL_BSW_ADC_GetValue(ADC_HandleTypeDef* hadc)
{
	return hadc->Instance->DR;
}
