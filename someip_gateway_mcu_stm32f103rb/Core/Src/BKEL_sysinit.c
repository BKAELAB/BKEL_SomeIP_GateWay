/*
 * BKEL_sysinit.c
 *
 *  Created on: Dec 20, 2025
 *      Author: PNYcom
 */
#include "main.h"

/* DEFINES For PWM & TIMER */
/* TIMER & CLK*/
#define	TIM_PSC_VALUE_72	(72U - 1U)
#define PWM_ARR_VALUE_1KHZ 	(1000U - 1U)
/* PIN */
#define PIN_PWM_OUT				(0U)	/* GPIOx PIN0 : PWM OUT 			*/
#define PIN_PWM_IN 				(6U)	/* GPIOx PIN6 : PWM IN  			*/
/* GPIO ALT MUX */
#define BIT_CLEAR 				(0xF)	/* 4-bit BIT CLEAR ~(BIT_CLEAR) 	*/
#define ALT_INPUT_FLOATING 		(0x4)	/* 4-bit ALT INPUT FLOATING 0b0010  */
#define ALT_PUSH_PULL 			(0xB)	/* 4-bit ALT PUSH PULL 0b1011		*/
/* SET VALUES */
#define PWM_MODE_1				(6U)	/* PWM MODE 1 (In OC1M , set 110)	*/
#define PWM_DUTY_PERCENT_0		(0U)	/* DUTY 0% 						*/
#define PWM_DUTY_PERCENT_50		(0.5)	/* DUTY 50% 						*/
#define PWM_DUTY_PERCENT_100	(1.0)	/* DUTY 100% 						*/


ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);

// 25.12.27 SJKANG
static void BKEL_PWM_Init(void);

#ifdef USE_UART_DEBUG
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t*)&"\r", 1, HAL_MAX_DELAY);
    return len;
}
#endif

/* sysinit */
void system_init(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_ADC1_Init();
	BKEL_PWM_Init();
}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }


}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig = {0};

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}



static void BKEL_PWM_Init(void)
{
	/* TIM2 / TIM3 Clock Enable */

	RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN);
	/* pin map */
	/* PA0 : TIM2_1 PWM_OUT */
	GPIOA->CRL &= ~(BIT_CLEAR << (PIN_PWM_OUT * 4));
	GPIOA->CRL |= (ALT_PUSH_PULL << (PIN_PWM_OUT * 4));
	/* PA6 : TIM3_1 PWM_IN */
	GPIOA->CRL &= ~(BIT_CLEAR << (PIN_PWM_IN * 4));
	GPIOA->CRL |= (ALT_INPUT_FLOATING << (PIN_PWM_IN * 4));

	TIM2->CR1 = BKEL_U_CLR;		// TIM2 Disable
	TIM3->CR1 = BKEL_U_CLR; 	// TIM3 Disable

	/* PWM OUT Setting */
	// 1. config Prescaler & ARR
	TIM2->PSC = TIM_PSC_VALUE_72;		// 1MHz (72MHz / 72)
	TIM2->ARR = PWM_ARR_VALUE_1KHZ;		// 1kHz PWM

	// 2. set PWM MODE
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
	TIM2->CCMR1 |= (PWM_MODE_1 << TIM_CCMR1_OC1M_Pos);	// PWM MODE 1
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE;						// preload enable

	// 3. set Duty value
	TIM2->CCR1 = PWM_ARR_VALUE_1KHZ * PWM_DUTY_PERCENT_50 + 0.5f; 	// 50%

	// 4. Enable Output
	TIM2->CCER |= TIM_CCER_CC1E;

	// 5. Update Event
	TIM2->EGR  |= TIM_EGR_UG;

	// 6. Counter Enable
	TIM2->CR1  |= BKEL_U_SET;	// TIM2 Enable

	/* PWM IN Setting */
	// 1. config Prescaler & ARR
	TIM3->PSC = TIM_PSC_VALUE_72;
	TIM3->ARR = 0xFFFF;			// MAX VALUE

	// 2. Input Capture Mapping
	TIM3->CCMR1 &= ~TIM_CCMR1_CC1S;
	TIM3->CCMR1 |= TIM_CCMR1_CC1S_0;	// CH1 <- TI1 (Period)
	TIM3->CCMR1 |= TIM_CCMR1_CC2S_1;	// CH2 <- TI1 (Thigh)

	// 3. Select Edge & Slave mode
	TIM3->CCER &= ~TIM_CCER_CC1P;		// CH1: Rising edge
	TIM3->CCER |= TIM_CCER_CC2P;		// CH2: Falling Edge

	TIM3->SMCR &= ~TIM_SMCR_TS;
	TIM3->SMCR |= TIM_SMCR_TS_2 | TIM_SMCR_TS_0;
	TIM3->SMCR &= ~TIM_SMCR_SMS;
	TIM3->SMCR |= TIM_SMCR_SMS_2;

	// 4. Capture Enable
	TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;

	// 5. Counter Enable
	TIM3->CR1 |= TIM_CR1_CEN;
}
