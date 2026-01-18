/*
 * BKEL_UDigno.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_APP_service.h"
#include "string.h"
#include "BKEL_BSW_uart.h"


static const char *service_advertise_payload[] =
{
	"SOF",

    "0x10 : RPC_LD2_Control",
    "0x11 : RPC_MCU_Reset",
    "0x12 : RPC_SPI_Read",
    "0x13 : RPC_PWM_SetOut",

    "0x20 : DIAG_PWM_Output_Value",
    "0x21 : DIAG_PWM_Input_Value",
    "0x22 : DIAG_ADC1_GetValue",
    "0x23 : DIAG_ADC2_GetValue",
    "0x24 : DIAG_GPO_PinState",
    "0x25 : DIAG_GPI_PinState",
    "0x26 : DIAG_LD2_PinState",

	"EOF"
};


void AppService_SendAdvertise(void)
{
    uint8_t tx_buf[256];

    for (size_t i = 0;
         i < (sizeof(service_advertise_payload) / sizeof(service_advertise_payload[0]));
         i++)
    {
        const char *payload = service_advertise_payload[i];
        uint16_t payload_len = strlen(payload);

        size_t packet_len = build_frame(
            tx_buf,
            sizeof(tx_buf),
            SERVICE_ADVERTISE,      // SID = 0x01
            P_DATA_TYPE_CHAR,       // ★ CHAR 타입
            (const uint8_t *)payload,
            payload_len
        );

        if (packet_len == 0)
            continue;

        BKEL_UART_Tx(&tx_buf, packet_len);
    }
}

