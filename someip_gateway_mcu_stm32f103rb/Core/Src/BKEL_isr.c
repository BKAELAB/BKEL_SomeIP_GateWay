/*
 * BKEL_isr.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include <BKEL_BSW_uart.h>
#include "BKEL_typedef.h"

// 26.01.05 Write by Panho
#include "BKEL_externs.h"
#include "stream_buffer.h"

void USART1_IRQHandler(void)
{
    /* IDLE flag 확인 (SR bit4) */
    if (PAN_USART1_SR & (1U << 4))
    {
        volatile uint32_t tmp;

        /* IDLE flag clear sequence */
        tmp = PAN_USART1_SR;
        tmp = PAN_USART1_DR;
        (void)tmp;

        /* DMA RX 중단 */
        DMA1_Channel5->CCR &= ~(1U << 0);

        /* 수신된 데이터 길이 계산 */
        uint16_t received_len = UART_RX_BUF_SIZE - DMA1_Channel5->CNDTR;

        if (received_len > 0)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;

            xStreamBufferSendFromISR(
                rxStream,
                (void *)uart1_rx_dma_buf,
                received_len,
                &xHigherPriorityTaskWoken
            );

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        /* DMA 재장전 */
        DMA1_Channel5->CNDTR = UART_RX_BUF_SIZE;
        DMA1_Channel5->CCR |= (1U << 0);
    }
}

void USART2_IRQHandler(void)
{
	if (PAN_USART2_SR & PAN_USART_CR1_IDLEIE)
	{
        // IDLE 플래그 클리어: SR 읽기 후 DR 읽기
        volatile uint32_t temp;
        temp = PAN_USART2_SR;
        temp = PAN_USART2_DR;
        (void)temp;

        // DMA 수신 중단 및 데이터 길이 계산
        DMA1_Channel6->CCR &= ~(1U << 0);
        uint16_t received_len = UART_RX_BUF_SIZE - DMA1_Channel6->CNDTR;

        if (received_len > 0) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // 데이터를 스트림 버퍼로 복사 (생성자)
            xStreamBufferSendFromISR(rxStream,
                                     (void *)uart2_rx_dma_buf,
                                     received_len,
                                     &xHigherPriorityTaskWoken);

            // 컨텍스트 스위칭 필요 여부 확인
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        // DMA 다시 설정 및 시작
        DMA1_Channel6->CNDTR = UART_RX_BUF_SIZE;
        DMA1_Channel6->CCR |= (1U << 0);
    }
}


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
