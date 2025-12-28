/*
 * BKEL_sysinit.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */
#include "main.h"

/* DEFINES For CLOCK */
/* FLASH 설정 */
#define FLASH_ACR_PRFTBE_EN         (1U << 4)       // Prefetch buffer enable
#define FLASH_LATENCY_2WS           (0x2U) 			// Two wait states, if 48 MHz < SYSCLK <= 72 MHz

/* RCC CR 비트*/
#define RCC_CR_HSEON_EN         	(1U << 16)   // HSE ON
#define RCC_CR_HSERDY_FLAG      	(1U << 17)   // HSE ready
#define RCC_CR_PLLON_EN         	(1U << 24)   // PLL enable
#define RCC_CR_PLLRDY_FLAG      	(1U << 25)   // PLL ready

/* RCC CFGR 비트 */
#define RCC_CFGR_PLLSRC_HSE       	(1U << 16)
#define RCC_CFGR_PLLMUL9          	(7U << 18)     // PLL x9 (7 = 9 - 2)
#define RCC_CFGR_HPRE_DIV1_VAL      (0x0U << 4)    // AHB Prescaler = /1
#define RCC_CFGR_PPRE1_DIV2_VAL     (0x4U << 8)    // APB1 Prescaler = /2
#define RCC_CFGR_PPRE2_DIV1_VAL     (0x0U << 11)   // APB2 Prescaler = /1
#define RCC_CFGR_SW_PLL_VAL         (0x2U << 0)    // SYSCLK = PLL
#define RCC_CFGR_SWS_PLL_STATUS     (0x2U << 2)    // SYSCLK status = PLL

/* BIT CLEAR */
#define FLASH_LATENCY_CLEAR		  	(0x7 << 0)	   // 3-bit clear
#define RCC_CFGR_PLLSRC_CLEAR     	(1U << 16)
#define RCC_CFGR_PLLMUL_CLEAR     	(0xFU << 18)
#define RCC_CFGR_HPRE_CLEAR       	(0xFU << 4)
#define RCC_CFGR_PPRE1_CLEAR      	(0x7U << 8)
#define RCC_CFGR_PPRE2_CLEAR      	(0x7U << 11)
#define RCC_CFGR_SW_CLEAR         	(0x3U << 0)
#define RCC_CFGR_SWS_CLEAR        	(0x3U << 2)


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

// 25.12.28 Hwang SeokJUN
static void BKEL_CLK_Init(void);

//void SystemClock_Config(void);
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
	// SystemClock_Config();
	BKEL_CLK_Init();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_ADC1_Init();
	BKEL_PWM_Init();
}



/**
  * @brief System Clock Configuration
  * @retval None
  */

static void BKEL_CLK_Init(void)
{
	// FLASH Latency 설정 (72MHz를 위해 2 Wait State 필수)
	FLASH->ACR |= (FLASH_ACR_PRFTBE_EN);				// Prefetch buffer enable
	FLASH->ACR &= ~(FLASH_LATENCY_CLEAR);          	    // Latency 비트 초기화
	FLASH->ACR |= (FLASH_LATENCY_2WS);              	// 2 Wait States
												    	// MCU가 플래시 메모리로부터 명령어 또는 데이터를 읽을 때, 2클럭만큼 기다린 뒤 값을 사용

	RCC->CR |= (RCC_CR_HSEON_EN);  					// HSEON (HSE Clock enable)
	while (!(RCC->CR & (RCC_CR_HSERDY_FLAG)));    	// HSE 오실레이터가 안정되었음을 나타내기 위해 하드웨어에 의해 설정
	    											// Oscillator(오실레이터): 주기적인 신호(사인파, 사각파)를 생성하는 장치

	// PLL 설정,  72 MHz maximum frequency (datasheet 1p)
	RCC->CFGR &= ~((RCC_CFGR_PLLSRC_HSE) | (RCC_CFGR_PLLMUL_CLEAR)); 			// PLLSRC , PLLMUL 0으로 초기화
    RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE);                  						// PLLSRC : PREDIV1 -> 103RB 보드에 없음. HSE로 사용
    RCC->CFGR |= (RCC_CFGR_PLLMUL9);                  							// PLLMUL : PLL input clock x 9
	    										 	 	 	 	 	 	 	 	// HSE = 8 MHz, PLLMUL = x9 → PLLCLK = 72 MHz

    // HPRE (AHB prescaler)
    RCC->CFGR &= ~(RCC_CFGR_HPRE_CLEAR);            // HPRE bit clear

	// PPRE1 (APB Low-speed prescaler (APB1))
    RCC->CFGR &= ~(RCC_CFGR_PPRE1_CLEAR);           // PPRE1 초기화
    RCC->CFGR |= (RCC_CFGR_PPRE1_DIV2_VAL);         // HCLK divided by 2

    // PPRE2 (APB high-speed prescaler (APB2)
    RCC->CFGR &= ~(RCC_CFGR_PPRE2_CLEAR);    		// PPRE2 초기화
    RCC->CFGR |= (RCC_CFGR_PPRE2_DIV1_VAL);			// HCLK not divided

    // PLL 활성화
    RCC->CR |= (RCC_CR_PLLON_EN);               	// PLLON
    while (!(RCC->CR & (RCC_CR_PLLRDY_FLAG)));    	// PLLRDY

    // SW (System clock Switch)
    // HSE는 PLL의 입력 클럭 역할, PLL은 실제로 시스템 전체를 구동하는 SYSCLK을 제공
    RCC->CFGR &= ~(RCC_CFGR_SW_CLEAR);           								// SW 초기화
    RCC->CFGR |= (RCC_CFGR_SW_PLL_VAL);            								// PLL selected as system clock
    while ((RCC->CFGR & (RCC_CFGR_SWS_CLEAR)) != (RCC_CFGR_SWS_PLL_STATUS));	// 시스템 클럭이 실제로 PLL로 바뀌었는지 확인
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
