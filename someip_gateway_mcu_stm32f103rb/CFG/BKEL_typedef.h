/*
 * BKEL_typedef.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_TYPEDEF_H_
#define INC_BKEL_TYPEDEF_H_

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

// Redefine Keyword
#define EXTERN extern

// typedef Structures
typedef uint8_t 	uI8_T;
typedef uint16_t 	uI16_T;
typedef uint32_t 	uI32_T;
typedef uint64_t 	uI64_T;

/* Driver Typedef */
typedef ADC_HandleTypeDef hADC_t;
typedef UART_HandleTypeDef hUART_t;

/* FreeRTOS Typedef */
typedef TaskHandle_t hTASK_t;

/* Custom defines -----------------------------------------------------------*/
#define B1_Pin 				GPIO_PIN_13
#define B1_GPIO_Port 		GPIOC
#define B1_EXTI_IRQn 		EXTI15_10_IRQn
#define USART_TX_Pin 		GPIO_PIN_2
#define USART_TX_GPIO_Port 	GPIOA
#define USART_RX_Pin 		GPIO_PIN_3
#define USART_RX_GPIO_Port 	GPIOA
#define LD2_Pin 			GPIO_PIN_5
#define LD2_GPIO_Port 		GPIOA
#define TMS_Pin 			GPIO_PIN_13
#define TMS_GPIO_Port 		GPIOA
#define TCK_Pin 			GPIO_PIN_14
#define TCK_GPIO_Port 		GPIOA
#define SWO_Pin 			GPIO_PIN_3
#define SWO_GPIO_Port 		GPIOB

#endif /* INC_BKEL_TYPEDEF_H_ */
