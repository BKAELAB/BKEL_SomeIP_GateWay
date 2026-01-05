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
		while(!(PAN_USART2_SR & (1 << 7)));
		PAN_USART2_DR = buf[i];
	}
}

