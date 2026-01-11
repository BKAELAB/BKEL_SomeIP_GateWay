/*
 * BKEL_isr.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include <BKEL_BSW_uart.h>
#include "BKEL_typedef.h"

// 26.01.05 Panho //////////////////////////////////////////////////////
#include "BKEL_externs.h"
#include "stream_buffer.h"

extern StreamBufferHandle_t xStreamBuffer;

volatile uint8_t uart_rx_flag = 0;
volatile uint16_t uart_rx_len = 0;

//void USART2_IRQHandler(void)
//{
//    if (PAN_USART2_SR & (0x01 << 4)) // SR 4
//    {
//        volatile uint32_t temp;
//        temp = PAN_USART2_SR;
//        temp = PAN_USART2_DR;
//        (void)temp;
//
//        /* rx data length */
//        uint16_t rx_length = 256 - PAN_DMA1_CNDTR;
//
//    }
//}


// 위의 코드를 바탕으로 isr 코드 구현
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
            xStreamBufferSendFromISR(xStreamBuffer,
                                     (void *)uart_rx_dma_buf,
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
////////////////////////////////////////////////////////////////////////

void vUART_VerifyTask(void *pvParameters) {
    uint8_t uart_rx_dma_buf[UART_RX_BUF_SIZE];
    size_t xReceivedBytes;

    while (1) {
        // 1. 스트림 버퍼로부터 데이터 수신 대기 (최대 2초)
        xReceivedBytes = xStreamBufferReceive(xStreamBuffer,
                                              (void *)uart_rx_dma_buf,
                                              sizeof(uart_rx_dma_buf),
                                              pdMS_TO_TICKS(2000));

        if (xReceivedBytes > 0) {
            // 2. Putty로 데이터 출력 (에코백)
        	BKEL_UART_Tx(uart_rx_dma_buf, UART_RX_BUF_SIZE);

        } else {
            // 데이터가 오지 않을 때 (디버깅용)
            // printf("Waiting for UART IDLE interrupt...\n");
        }
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
