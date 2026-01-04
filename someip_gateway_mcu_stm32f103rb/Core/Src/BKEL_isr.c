/*
 * BKEL_isr.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_typedef.h"

// 26.01.05 Panho //////////////////////////////////////////////////////
#include "BKEL_uart.h"
#include "BKEL_externs.h"

volatile uint8_t uart_rx_flag = 0;
volatile uint16_t uart_rx_len = 0;

void USART2_IRQHandler(void)
{
    if (PAN_USART2_SR & (0x01 << 4)) // SR 4
    {
        volatile uint32_t temp;
        temp = PAN_USART2_SR;
        temp = PAN_USART2_DR;
        (void)temp;

        /* rx data length */
        uint16_t rx_length = 256 - PAN_DMA1_CNDTR;

        /* tx */
        if (rx_length > 0)
        {
        	for(int i = 0; i < rx_length; i++)
			{
				// TXE: 7번 비트 -> 1 될 때까지
				while(!(PAN_USART2_SR & (1 << 7)));

				// echo
				PAN_USART2_DR = uart_rx_dma_buf[i];
			}
        }
    }
}
////////////////////////////////////////////////////////////////////////

// ISR for GPIO EXTI 15:10
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

}

// brief  This function is executed in case of error occurrence.
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
}
