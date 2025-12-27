/*
 * BKEL_BSW_pwm.h
 *
 *  Created on: Dec 27, 2025
 *      Author: seokjun.kang
 */

#ifndef BSW_INC_BKEL_BSW_PWM_H_
#define BSW_INC_BKEL_BSW_PWM_H_

#include  "stdint.h"

void BKEL_PWM_SetDuty(uint8_t duty_percent);
uint16_t BKEL_PWM_ReadDuty(void);

#endif /* BSW_INC_BKEL_BSW_PWM_H_ */
