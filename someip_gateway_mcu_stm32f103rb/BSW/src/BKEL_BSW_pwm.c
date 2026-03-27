/*
 * BKEL_BSW_pwm.c
 *
 *  Created on: Dec 27, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_BSW_pwm.h"
#include "main.h"

void BKEL_PWM_SetPeriod(uint8_t period)
{
    if (period == 0) return;
	TIM2->ARR = 1000000 / (period * 1000);
}

void BKEL_PWM_SetDuty(uint8_t duty_percent)
{
	if (duty_percent > 100) duty_percent = 100;

	TIM2->CCR1 = (TIM2->ARR + 1) * duty_percent / 100;
}

uint8_t BKEL_PWM_ReadDuty(void)
{
	uint16_t period = TIM2->ARR;
	uint16_t Thigh = TIM2->CCR1;
	uint16_t duty = 999;
	if (period > 0)
	{
		duty = (Thigh * 100U) / period;
	}
	return (uint8_t)duty;
}

uint8_t BKEL_PWM_ReadPeriod(void)
{
	uint16_t period = TIM2->ARR;
    if (period > 0) period = 1000000 / period;
	return (uint8_t)(period/999U);
}

void BKEL_PWM_Input_Read_TIM3(BKEL_BSW_PwmInput_t *pOut)
{
    uint32_t period;
    uint32_t high;

    if (pOut == NULL)
        return;

    /* CCR1: Period (Rising → Rising)
       CCR2: High time (Rising → Falling) */
    period = TIM3->CCR1;
    high   = TIM3->CCR2;

    if ((period == 0U) || (high > period))
    {
        pOut->valid = 0U;
        return;
    }

    pOut->period_cnt = period;
    pOut->high_cnt   = high;

    /* Duty */
    pOut->duty_percent = ((float)high / (float)period) * 100.0f;

    /* Frequency */
    pOut->freq_hz =
        (float)72000000UL /
        ((float)71U * (float)period);

    pOut->valid = 1U;
}
