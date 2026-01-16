/*
 * taskinfo.h
 *
 *  Created on: Jan 16, 2026
 *      Author: Choo
 */

#ifndef INC_BKEL_BSW_DIAGNOSTIC_H_
#define INC_BKEL_BSW_DIAGNOSTIC_H_

// 명령어 ID 정의
typedef enum {
    CMD_IDLE = 0,
    CMD_MOTOR_START,
	CMD_BATTERY_CHECK,
    CMD_MOTOR_STOP,
    CMD_SENSOR_READ
} CommandID_t;

/* 명령어 구조체 */
typedef struct {
    uint8_t  cmdID;     // 명령어 종류
    uint32_t parameter; // 부가 정보
} CommandMsg_t;

/* 진단 결과 구조체 */
typedef struct {
    uint8_t  diagID;    // 진단 대상 ID
    uint16_t errorCode; // 에러 코드 (0이면 정상)
    float    value;     // 진단 수치 (전압, 전류 등)
} DiagResult_t;


/* 큐 핸들 (외부 노출) */
extern osMessageQueueId_t commandQueueHandle;
extern osMessageQueueId_t diagQueueHandle;

#endif
