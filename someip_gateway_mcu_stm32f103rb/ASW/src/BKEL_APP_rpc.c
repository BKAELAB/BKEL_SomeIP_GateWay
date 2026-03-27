/*
 * BKEL_APP_rpc.c
 *
 *  Created on: Jan 12, 2026
 *      Author: dahyun
 */
#include "BKEL_APP_rpc.h"
#include "BKEL_BSW_led.h"
#include "BKEL_APP_protocol.h"
#include "BKEL_BSW_pwm.h"
#include "BKEL_BSW_spi.h"

#define ENABLE_MCU_RESET

// sid 0x10
void BKEL_RPC_LD2_Control(BKEL_Common_Packet_t *packet)
{
	BKEL_LD2_Cmd_t cmd = (BKEL_LD2_Cmd_t)packet->payload[0];

	switch(cmd)
	{
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

		default:
		    printf("[LD2] invalid cmd: 0x%02X\r\n", packet->payload[0]);
		    break;
	}
}

// sid 0x11
void BKEL_RPC_MCU_Reset(BKEL_Common_Packet_t *packet)
{
	BKEL_MCU_Cmd_t cmd = (BKEL_MCU_Cmd_t)packet->payload[0];

	switch(cmd)
	{
		case MCU_CMD_RESET:
			//[DEBUG]
			printf("[MCU_Reset]\r\n");
#if defined(ENABLE_MCU_RESET)
			NVIC_SystemReset();
#else
			printf("[MCU_Reset] disable\r\n");
#endif
			break;

		default:
			printf("[MCU] invalid cmd: 0x%02X\r\n", packet->payload[0]);
			break;
	}
}

// sid 0x12
void BKEL_RPC_SPI_Read(BKEL_Common_Packet_t *packet)
{
	BKEL_SPI_Cmd_t cmd = (BKEL_SPI_Cmd_t)packet->payload[0];

	switch(cmd)
	{

		case SPI_CMD_READ:
		{
			uint8_t rx[4];

			//[DEBUG]
			printf("[SPI Read]\r\n");
			for (int i=0; i < 4; i++)
			{
				rx[i] = BKEL_SPI2_Transfer(0x00); // dummy
				printf("RX = 0x%02X\r\n", rx[i]);
			}
			break;
		}

		case SPI_CMD_WRITE:
		{
			//[DEBUG]
			printf("[SPI Write]\r\n");
			for (int i = 0; i < 4; i++)
			{
				uint8_t tx = packet->payload[i + 1U];
				uint8_t rx = BKEL_SPI2_Transfer(tx);
			    printf("TX = 0x%02X, RX(loopback) = 0x%02X\r\n", tx, rx);
			}
			break;
		}

		default:
		{
			printf("[SPI] invalid cmd: 0x%02X\r\n", packet->payload[0]);
			break;
		}
	}
}

// sid0x13
void BKEL_RPC_PWM_Setout(BKEL_Common_Packet_t *packet)
{
	//[DEBUG]
	printf("[PWM Setout]\r\n");
	BKEL_PWM_Payload_t *pwm_payload =
			(BKEL_PWM_Payload_t *)packet->payload;

	BKEL_PWM_SetPeriod(pwm_payload->period);
	BKEL_PWM_SetDuty(pwm_payload->duty);
}
