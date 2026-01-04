/*
 * BKEL_APP_protocol.h
 *
 *  Created on: Jan 4, 2026
 *      Author: sjkang
 */

#ifndef ASW_INC_BKEL_APP_PROTOCOL_H_
#define ASW_INC_BKEL_APP_PROTOCOL_H_

#include "BKEL_typedef.h"

/* PROTOCOL DEFINES */
#define SOF_DATA_VALUE 			(0xAAU)

/* SIDs */
#define SERVICE_ADVERTISE 		(0x01U)
#define RPC_LD2_CONTROL 		(0x10U)
#define RPC_MCU_RESET 			(0x11U)
#define RPC_SPI_READ			(0x12U)
#define RPC_PWM_SETOUT			(0x13U)
#define DIAG_PWM_OUTPUT_VALUE	(0x20U)
#define DIAG_PWM_INPUT_VALUE	(0x21U)
#define DIAG_ADC1_GET_VALUE		(0x22U)
#define DIAG_ADC2_GET_VALUE		(0x23U)
#define DIAG_GPO_PINSTATE		(0x24U)
#define DIAG_GPI_PINSTATE		(0x25U)
#define DIAG_LD2_PINSTATE		(0x26U)

/* Types */
#define P_DATA_TYPE_UI8			(0x01U)
#define P_DATA_TYPE_UI16		(0x02U)
#define P_DATA_TYPE_CHAR		(0x03U)

/* DLC */
#define DLC_VARIABLE			(0xFFU)
#define DLC_STATIC_1B			(0x01U)
#define DLC_STATIC_2B			(0x02U)
#define DLC_STATIC_5B			(0x05U)

/* PAYLOAD_OPCODE */
#define LD2_OPCODE_OFF			(0x00U)
#define LD2_OPCODE_ON			(0x01U)
#define LD2_OPCODE_TOGGLE		(0x02U)
#define MCU_OPCODE_RESET		(0x01U)
#define SPI_OPCODE_READ			(0x00U)
#define SPI_OPCODE_WRITE		(0x01U)

/* STRUCT */
#pragma pack(push,1)
typedef struct {
	uint8_t sid;
	uint8_t type;
	uint16_t dlc;
}BKEL_Data_Frame_Header_t;
#pragma pack(pop)

typedef enum {
	PARSE_OK,
	PARSE_INCOMPLETE,
	PARSE_INVALID
}BKEL_PARSE_RESULT_e;


EXTERN size_t build_frame( uint8_t *out_buf,
						 size_t   out_buf_size,
						 uint8_t  sid,
						 uint8_t  type,
						 const uint8_t *payload,
						 uint16_t payload_len);

EXTERN void parse_packet(uint8_t *buf, size_t *len);



#endif /* ASW_INC_BKEL_APP_PROTOCOL_H_ */
