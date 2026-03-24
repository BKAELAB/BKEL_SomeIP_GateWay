/*
 * BKEL_APP_pwm.c
 *
 *  Created on: Dec 27, 2025
 *      Author: sjkang
 */

#include "BKEL_APP_pwm.h"
#include "BKEL_BSW_pwm.h"

void AppPwmTest(void)
{
	static uint8_t duty = 0;
	BKEL_PWM_SetDuty(duty);
	duty = duty < 100 ? duty + 10 : 0U;
	uint8_t result = BKEL_PWM_ReadDuty();
	// printf("PWM TEST. Duty : %d\r\n" , result);
}
