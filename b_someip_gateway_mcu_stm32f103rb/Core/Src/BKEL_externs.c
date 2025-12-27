/*
 * BKEL_externs.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_externs.h"

/* Extern VARS */

QueueHandle_t hQueue;

hTASK_t hSendAdvertiseTask;
hTASK_t hCommandCustomerTask;
hTASK_t hSendDataTask;
hTASK_t hRPCTask;
