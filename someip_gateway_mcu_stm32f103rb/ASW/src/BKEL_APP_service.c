/*
 * BKEL_UDigno.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_APP_service.h"
#include "string.h"



#if USE_FEATURE_TEST

void uart_hex_dump(UART_HandleTypeDef *huart,
                   const uint8_t *buf,
                   size_t len)
{
    char line[8];  // "FF " + '\0'

    for (size_t i = 0; i < len; i++)
    {
        int n = snprintf(line, sizeof(line), "%02X ", buf[i]);
        HAL_UART_Transmit(huart, (uint8_t*)line, n, 100);
    }

    HAL_UART_Transmit(huart, (uint8_t*)"\r\n", 2, 100);
}

EXTERN void AppServiceTest()
{
	uint8_t out_buf[128];
	size_t out_buf_size = 128;
	uint8_t sid = SERVICE_ADVERTISE;
	uint8_t type = P_DATA_TYPE_CHAR;
	const char* payload = "Service List \r\n";

	uint16_t payload_len = strlen(payload);
	uint16_t packet_size = build_frame(out_buf, out_buf_size, sid, type, (const uint8_t*)payload, payload_len);

	uart_hex_dump(&huart2, (uint8_t*)out_buf, packet_size);
}
#endif
