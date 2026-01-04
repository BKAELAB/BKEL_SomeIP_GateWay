/*
 * BKEL_isr.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_typedef.h"
#include "BKEL_sysinit.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"

extern TaskHandle_t xUartTaskHandle;

xTaskHandle xUartTaskHandle;

// ISR for GPIO EXTI 15:10
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

}

void EXTI3_IRQHandler(void)
{
    if (EXTI->PR & (1 << 3))
    {
        EXTI->PR = (1 << 3);
        EXTI->IMR &= ~(1 << 3);

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xUartTaskHandle != NULL)
        {
            vTaskNotifyGiveFromISR(xUartTaskHandle, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
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
