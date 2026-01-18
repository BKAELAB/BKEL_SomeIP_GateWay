/*
 * BKEL_adc.c
 *
 *  Created on: Jan 16, 2026
 *      Author: CHS
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;
TaskHandle_t xDiagTaskHandle = NULL;

// 1. LED 상태를 추적할 변수 (하드웨어 읽기 대신 사용)
static uint8_t software_led_state = 0;

// 1. 하트비트 태스크 (상태를 직접 관리)
void StartHeartbeatTask(void *argument) {
    for(;;) {
        // 소프트웨어 변수 토글 (0 -> 1 -> 0 ...)
        software_led_state = !software_led_state;

        // 변수 값에 따라 LED 물리 제어
        if (software_led_state) {
        	BKEL_LD2_On();
        } else {
        	BKEL_LD2_Off();
        }

        // 진단 태스크로 현재 상태(0 또는 1) 전송
        if (xDiagTaskHandle != NULL) {
            xTaskNotify(xDiagTaskHandle, (uint32_t)software_led_state, eSetValueWithOverwrite);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 2. 진단 태스크 (안전한 전송 방식 사용)
void StartDiagnosticTask(void *argument) {
    uint32_t rxVal;
    char tx_buffer[128];

    for(;;) {
        // 모든 비트를 클리어하며 대기
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &rxVal, portMAX_DELAY) == pdTRUE) {

            // 수신된 값 확인을 위해 로그 구성
            if (rxVal == 1) {
                snprintf(tx_buffer, sizeof(tx_buffer), ">>> STATUS: LED IS ON\r\n");
            } else {
                snprintf(tx_buffer, sizeof(tx_buffer), ">>> STATUS: LED IS OFF\r\n");
            }

            BKEL_UART_Tx((uint8_t*)tx_buffer, strlen(tx_buffer));
        }
    }
}



