/*
 * BKEL_APP_rpc.h
 *
 *  Created on: Jan 12, 2026
 *      Author: dahyun
 */

#ifndef ASW_INC_BKEL_APP_RPC_C_
#define ASW_INC_BKEL_APP_RPC_C_

#include "BKEL_APP_protocol.h"

typedef enum {
	SID_LED_CONTROL		= RPC_LD2_CONTROL,
	SID_MCU_RESET		= RPC_MCU_RESET,
	SID_SPI_READ		= RPC_SPI_READ,
	SID_PWM_SETOUT		= RPC_PWM_SETOUT
} BKEL_SID_t;

typedef enum {
	LD2_CMD_OFF			= LD2_OPCODE_OFF,
	LD2_CMD_ON			= LD2_OPCODE_ON,
	LD2_CMD_TOGGLE		= LD2_OPCODE_TOGGLE
} BKEL_LD2_Cmd_t;

typedef enum {
	MCU_CMD_RESET 		= MCU_OPCODE_RESET
} BKEL_MCU_Cmd_t;

typedef enum {
	SPI_CMD_READ  		= SPI_OPCODE_READ,
	SPI_CMD_WRITE 		= SPI_OPCODE_WRITE
} BKEL_SPI_Cmd_t;

typedef struct {
    uint8_t duty;     // 1 Byte
    uint8_t period;   // 1 Byte (kHz)
} BKEL_PWM_Payload_t;

void BKEL_RPC_LD2_Control(BKEL_Common_Packet_t *packet);
void BKEL_RPC_MCU_Reset(BKEL_Common_Packet_t *packet);
void BKEL_RPC_SPI_Read(BKEL_Common_Packet_t *packet);
void BKEL_RPC_PWM_Setout(BKEL_Common_Packet_t *packet);

#endif /* ASW_INC_BKEL_APP_RPC_C_ */
