/*
 * BKEL_adc.c
 *
 *  Created on: Dec 20, 2025
 *      Author: PNYcom
 */

#include <BKEL_BSW_adc.h>

uint32_t BKEL_BSW_ADC_GetValue(ADC_HandleTypeDef* hadc)
{
	return hadc->Instance->DR;
}
