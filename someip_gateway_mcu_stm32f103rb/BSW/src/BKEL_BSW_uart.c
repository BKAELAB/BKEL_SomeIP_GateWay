/*
 * BKEL_BSW_uart.c
 *
 *  Created on: Jan 5, 2026
 *      Author: sjkang
 */

#include <BKEL_BSW_uart.h>

void BKEL_UART_Tx(const uint8_t *buf, uint16_t buf_len)
{
	for (uint16_t i = 0; i < buf_len; ++i)
	{
		while(!(PAN_USART1_SR & (1 << 7)));
		PAN_USART1_DR = buf[i];
	}
	for (uint16_t i = 0; i < buf_len; ++i)
	{
		while(!(PAN_USART2_SR & (1 << 7)));
		PAN_USART2_DR = buf[i];
	}
}

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
