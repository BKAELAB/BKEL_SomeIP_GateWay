#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

#include <BKEL_sysconfig.h>
#include <BKEL_externs.h>
#include <BKEL_typedef.h>
#include <BKEL_sysinit.h>

#include <BKEL_APP_TaskM.h>
#include <BKEL_APP_service.h>
#include <BKEL_APP_pwm.h>

#include <BKEL_BSW_gpio.h>
#include <BKEL_BSW_adc.h>

#ifdef USE_UART_DEBUG
#include "stdio.h"
#endif

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
