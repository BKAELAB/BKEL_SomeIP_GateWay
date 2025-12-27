/*
 * BKEL_isr.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#include "BKEL_typedef.h"

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
