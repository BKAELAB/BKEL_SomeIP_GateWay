/*
 * BKEL_externs.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_EXTERNS_H_
#define INC_BKEL_EXTERNS_H_

#include "BKEL_typedef.h"
#include "queue.h"

/* Externs */
EXTERN hADC_t  hadc1;
EXTERN hUART_t huart2;

EXTERN QueueHandle_t hQueue;
EXTERN hTASK_t hSendAdvertiseTask;
EXTERN hTASK_t hCommandCustomerTask;
EXTERN hTASK_t hSendDataTask;
EXTERN hTASK_t hRPCTask;

#endif /* INC_BKEL_EXTERNS_H_ */
