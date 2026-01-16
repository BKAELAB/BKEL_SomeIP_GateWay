/*
 * BKEL_adc.c
 *
 *  Created on: Jan 16, 2026
 *      Author: CHS
 */

#include "main.h"
#include <BKEL_BSW_diagnostic.h>

void StartHandleTask(void *argument) {
    CommandMsg_t receivedCmd;
    DiagResult_t diagResult;

    for(;;) {
        // [WAKE-UP] 명령어가 들어올 때까지 대기
        if (osMessageQueueGet(commandQueueHandle, &receivedCmd, NULL, osWaitForever) == osOK) {

            // 1. 명령어 분기 (Command Branch)
            switch(receivedCmd.cmdID) {
                case CMD_MOTOR_START:
                    // PC4(CH14)에 연결
                    diagResult.diagID = 0x14; // Channel 14 표시
                    diagResult.value = (float)adc_dma_buf[0] * (3.3f / 4095.0f); // 전압 변환

                    // 진단 결과 판정 (예: 2.5V 이상이면 과전류 에러)
                    diagResult.errorCode = (diagResult.value > 2.5f) ? 0x01 : 0x00;
                    break;

                case CMD_BATTERY_CHECK:
                    // PC5(CH15)에 연결
                    diagResult.diagID = 0x15; // Channel 15 표시
                    diagResult.value = (float)adc_dma_buf[1] * (3.3f / 4095.0f) * 5.0f; // 분압비 고려

                    // 진단 결과 판정 (예: 10V 미만이면 저전압 에러)
                    diagResult.errorCode = (diagResult.value < 10.0f) ? 0x02 : 0x00;
                    break;

                default:
                    diagResult.diagID = 0x00;
                    diagResult.errorCode = 0xFFFF;
                    diagResult.value = 0.0f;
                    break;
            }

            // 2. 진단 결과를 전송 태스크로 전달 (Toss)
            osMessageQueuePut(diagQueueHandle, &diagResult, 0, 0);
        }
    }
}

void StartDiagnosticDataSendTask(void *argument) {
    DiagResult_t res;
    char txBuffer[64];

    for(;;) {
        // [WAKE-UP] 진단 결과가 들어올 때까지 대기
        if (osMessageQueueGet(diagQueueHandle, &res, NULL, osWaitForever) == osOK) {

            // 1. 전송 데이터 포맷팅
            int len = snprintf(txBuffer, sizeof(txBuffer),
                               "[DIAG] CH:%d, ERR:%d, VAL:%.2f\r\n",
                               res.diagID, res.errorCode, res.value);

            // 2. USART2를 이용한 실제 전송
            if (len > 0) {
            	BKEL_UART_Tx((uint8_t*)txBuffer, len);
            }
        }
    }
}
