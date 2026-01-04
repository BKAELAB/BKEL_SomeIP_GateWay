/*
 * BKEL_externs.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_externs.h"

/* Extern VARS */

//QueueHandle_t hQueue;
uint8_t ucStreamBufferStorage[SB_SIZE];     // 실제 상자
StaticStreamBuffer_t xStreamBufferStruct;    // 상자 관리 정보
extern StreamBufferHandle_t xStreamBuffer;   // 리모컨(핸들)

hTASK_t hSendAdvertiseTask;
hTASK_t hCommandCustomerTask;
hTASK_t hSendDataTask;
hTASK_t hRPCTask;
