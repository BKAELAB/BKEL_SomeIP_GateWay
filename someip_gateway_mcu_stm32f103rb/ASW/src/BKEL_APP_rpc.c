/*
 * BKEL_APP_rpc.c
 *
 *  Created on: Jan 12, 2026
 *      Author: dahyun
 */
#include "BKEL_APP_rpc.h"
#include "BKEL_BSW_led.h"
#include "BKEL_APP_protocol.h"

//Proto
void rpc_ld2_control(BKEL_Common_Packet_t *packet);
void rpc_mcu_reset(BKEL_Common_Packet_t *packet);
void rpc_spi_read(BKEL_Common_Packet_t *packet);
void rpc_pwm_setout(BKEL_Common_Packet_t *packet);

//void handle_frame();

void rpc_ld2_control(BKEL_Common_Packet_t *packet)
{

	switch((BKEL_LD2_Payload_t)packet->payload[0]) {
		case LD2_CMD_OFF:
			printf("LD2 OFF\r\n");
			BKEL_LD2_Off();
			break;
		case LD2_CMD_ON:
			printf("LD2 ON\r\n");
			BKEL_LD2_On();
			break;
		case LD2_CMD_TOGGLE:
			printf("LD2 Toggle\r\n");
			BKEL_LD2_Toggle();
			break;
	}
}

void rpc_mcu_reset(BKEL_Common_Packet_t *packet)
{
	printf("[MCU_Reset]\r\n");
	switch((BKEL_MCU_Payload_t)packet->payload[0]) {
		case MCU_RESET:
			NVIC_SystemReset();
			break;
	}
}

void rpc_spi_read(BKEL_Common_Packet_t *packet)
{
	switch((BKEL_SPI_Cmd_t)packet->payload[0]) {

	case SPI_CMD_READ:
		printf("SPI Read\r\n");
		uint8_t rx[4];
		for (int i=0; i < 4; i++) {
			rx[i] = BKEL_SPI2_Transfer(0x00); // 더미데이터 보내줌
			//rx[i] = BKEL_SPI2_Transfer(packet->payload[0x00]);
			printf("RX = 0x%02X\r\n", rx[i]);
		}

		break;

	case SPI_CMD_WRITE:
		uint8_t tx[4];
		printf("SPI Write\r\n");

		for (int i = 1; i < 5; i++) {
			tx[i] = BKEL_SPI2_Transfer(packet->payload[i]);
			printf("TX = 0x%02X\r\n", tx[i]);
		}
		break;
	}
}

void rpc_pwm_setout(BKEL_Common_Packet_t *packet)
{
	uint8_t duty = packet->payload;
	uint8_t period = packet->payload[1];

	BKEL_PWM_SetDuty(duty);

}
