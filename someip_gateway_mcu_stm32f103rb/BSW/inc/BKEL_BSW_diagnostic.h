/*
 * taskinfo.h
 *
 *  Created on: Jan 16, 2026
 *      Author: Choo
 */

#ifndef INC_BKEL_BSW_DIAGNOSTIC_H_
#define INC_BKEL_BSW_DIAGNOSTIC_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* 1. 진단 이벤트 비트 정의 (명령어 분기용) */
#define EVENT_LED_OFF        (1 << 0)
#define EVENT_LED_ON         (1 << 1)

/* 2. 외부에서 접근 가능하도록 태스크 핸들 노출 */
extern TaskHandle_t xDiagTaskHandle;

/* 3. 함수 프로토타입 */
void StartDiagnosticTask(void *argument);
void StartHeartbeatTask(void *argument);

#endif
