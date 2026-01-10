/*
 * BKEL_UDigno.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_APP_service.h"
#include "string.h"
#include "BKEL_BSW_uart.h"


#if USE_FEATURE_TEST

void uart_hex_dump(UART_HandleTypeDef *huart,
                   const uint8_t *buf,
                   size_t len)
{
    char line[8];  // "FF " + '\0'

    for (size_t i = 0; i < len; i++)
    {
        int n = snprintf(line, sizeof(line), "%02X ", buf[i]);
        BKEL_UART_Tx((uint8_t*)line, n);
    }
    BKEL_UART_Tx((uint8_t*)"\r\n", 2);
}

static const char *service_advertise_payload[] =
{
    "0x10 : RPC_LD2_Control\r\n",
    "0x11 : RPC_MCU_Reset\r\n",
    "0x12 : RPC_SPI_Read\r\n",
    "0x13 : RPC_PWM_SetOut\r\n",

    "0x20 : DIAG_PWM_Output_Value\r\n",
    "0x21 : DIAG_PWM_Input_Value\r\n",
    "0x22 : DIAG_ADC1_GetValue\r\n",
    "0x23 : DIAG_ADC2_GetValue\r\n",
    "0x24 : DIAG_GPO_PinState\r\n",
    "0x25 : DIAG_GPI_PinState\r\n",
    "0x26 : DIAG_LD2_PinState\r\n"
};


void AppService_SendAdvertise(void)
{
    static size_t advertise_index = 0;
    uint8_t tx_buf[256];

    const char *payload = service_advertise_payload[advertise_index];
    uint16_t payload_len = strlen(payload);

    size_t packet_len = build_frame(
        tx_buf,
        sizeof(tx_buf),
        SERVICE_ADVERTISE,     // SID = 0x01
        P_DATA_TYPE_CHAR,      // CHAR 타입
        (const uint8_t *)payload,
        payload_len
    );

    if (packet_len != 0)
    {
        uart_hex_dump(&huart2, tx_buf, packet_len);
    }

    /* 다음 항목으로 이동 */
    advertise_index++;

    if (advertise_index >=
        (sizeof(service_advertise_payload) / sizeof(service_advertise_payload[0])))
    {
        advertise_index = 0;   // 끝까지 갔으면 다시 처음
    }
}

//EXTERN void AppServiceTest()
//{
//	uint8_t out_buf[128];
//	size_t out_buf_size = 128;
//	uint8_t sid = SERVICE_ADVERTISE;
//	uint8_t type = P_DATA_TYPE_CHAR;
//	const char* payload = "Service List \r\n";
//
//	uint16_t payload_len = strlen(payload);
//	uint16_t packet_size = build_frame(out_buf, out_buf_size, sid, type, (const uint8_t*)payload, payload_len);
//
//	uart_hex_dump(&huart2, (uint8_t*)out_buf, packet_size);
//}

#endif
