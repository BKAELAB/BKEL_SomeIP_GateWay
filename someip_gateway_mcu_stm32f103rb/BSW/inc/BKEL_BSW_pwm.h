/*
 * BKEL_BSW_pwm.h
 *
 *  Created on: Dec 27, 2025
 *      Author: seokjun.kang
 */

#ifndef BSW_INC_BKEL_BSW_PWM_H_
#define BSW_INC_BKEL_BSW_PWM_H_

#include <main.h>

typedef struct BKEL_BSW_PwmInput
{
    uint32_t period_cnt;
    uint32_t high_cnt;       // High time (timer count)
    float    duty_percent;   // Duty (%)
    float    freq_hz;        // Frequency (Hz)
    uint8_t  valid;
} BKEL_BSW_PwmInput_t;

#include  "stdint.h"

void BKEL_PWM_SetDuty(uint8_t duty_percent);
void BKEL_PWM_SetPeriod(uint8_t);
uint8_t BKEL_PWM_ReadDuty(void);
uint8_t BKEL_PWM_ReadPeriod(void);
void BKEL_PWM_Input_Read_TIM3(BKEL_BSW_PwmInput_t *pOut);

#endif /* BSW_INC_BKEL_BSW_PWM_H_ */
