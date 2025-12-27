/*
 * BKEL_BSW_pwm.c
 *
 *  Created on: Dec 27, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_BSW_pwm.h"
#include "main.h"

void BKEL_PWM_SetDuty(uint8_t duty_percent)
{
	if (duty_percent > 100) duty_percent = 100;

	TIM2->CCR1 = (TIM2->ARR + 1) * duty_percent / 100;
}

uint16_t BKEL_PWM_ReadDuty(void)
{
	uint16_t period = TIM3->CCR1;
	uint16_t Thigh = TIM3->CCR2;
	uint16_t duty = 999;
	if (period > 0)
	{
		duty = (Thigh * 100U) / period;
	}
	return duty;
}
