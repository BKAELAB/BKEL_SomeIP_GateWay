/*
 * BKEL_sysinit.c
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */
#include "main.h"

/* Variables */
volatile uint16_t adc_dma_buf[ADC_DMA_BUF_LEN];		// 프로그램 코드 외부에 있는 어떤 요인에 의해 변경될 수 있음

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


/* DEFINES For ADC & DMA */

/* RCC */
#define RCC_ADC_PRE        			14U
#define RCC_ADC_PRE_CLEAR         	0x3U
#define RCC_ADC_PRE_DIV6         	0x2U
#define RCC_IOPC_EN           		(1U << 4)
#define RCC_ADC1_EN           		(1U << 9)
#define RCC_DMA1_EN           		(1U << 0)

/* GPIO */
#define GPIO_ANALOG_MODE          	0xFU
#define GPIO_PC4            	 	16U
#define GPIO_PC5                 	20U

/* DMA */
#define DMA_EN		              	(1U << 0)
#define DMA_CIRCULAR_EN           	(1U << 5)
#define DMA_MINC_EN               	(1U << 7)
#define DMA_PSIZE_16BIT           	(1U << 8)
#define DMA_MSIZE_16BIT           	(1U << 10)

/* ADC  */
/* DR */
#define ADC1_DR_ADDR    			(0x4001244C)	// ADC1 : 0x4001 2400, ADC_DR : 0x4C

/* CR1 */
#define ADC_SCAN_EN           		(1U << 8)

/* CR2 */
#define ADC_POWER_ON              	(1U << 0)
#define ADC_CONT_EN           	  	(1U << 1)
#define ADC_CAL_EN                	(1U << 2)
#define ADC_RSTCAL_EN            	(1U << 3)
#define ADC_DMA_EN                	(1U << 8)
#define ADC_SWSTART_EN            	(1U << 22)

/* Sampling Time */
#define ADC_SMPR_CLEAR      		0x7U
#define ADC_SMPR_239_5CYC         	0x7U
#define ADC_SMPR_CH14      		  	12U
#define ADC_SMPR_CH15      		  	15U

/* Regular Sequence */
#define ADC_SQ_LEN         			(0xFU << 20)
#define ADC_SQ_LEN_2CH         		(1U << 20)
#define ADC_SQ1_POS              	0U
#define ADC_SQ2_POS              	5U

/* ADC Channels */
#define ADC_CH14                  	14U
#define ADC_CH15                  	15U

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
#define BKEL_INPUT_PULL			(0x8)	/* 4-bit PULL-UP/DOWN INPUT 0b1000	*/
#define BKEL_OUTPUT_PULL		(0x1)	/* 4-bit PULL-UP/DOWN OUTPUT 0b0001	*/
/* SET VALUES */
#define PWM_MODE_1				(6U)	/* PWM MODE 1 (In OC1M , set 110)	*/
#define PWM_DUTY_PERCENT_0		(0U)	/* DUTY 0% 						*/
#define PWM_DUTY_PERCENT_50		(0.5)	/* DUTY 50% 						*/
#define PWM_DUTY_PERCENT_100	(1.0)	/* DUTY 100% 						*/
/* GPIO PORT */
#define GPIOC_RESET				(1U << 4)	/* GPIOC RESET 	*/
#define GPIOA_RESET				(1U << 2)	/* GPIOA RESET 	*/
#define AFIO_RESET				(1U)		/* AFIO RESET	*/
/* GPIO PIN */
#define GPIO_PIN_LD2			(5U)		/* GPIOA PIN5: LD2 	*/
#define GPIO_PIN_INPUT			(0U)		/* GPIOC PIN0: INPUT */
#define GPIO_PIN_OUTPUT			(1U)		/* GPIOC PIN1: OUTPUT */
#define GPIO_PIN_B1				(13U-8U)	/* GPIOC PIN13: BTN1 */
#define EXTI_PIN13				(1U)		/* EXTI13 [7:4] */

ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

// 25.12.28 Hwang SeokJUN
static void BKEL_CLK_Init(void);

//void SystemClock_Config(void);
static void BKEL_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

// 26.01.02 Hwang SeokJun
static void BKEL_ADC1_DMA_Init(void);

// 25.12.27 SJKANG
static void BKEL_PWM_Init(void);

// 25.12.28 DHKWON
static void BKEL_GPIO_Init(void);

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
	BKEL_CLK_Init();
	BKEL_GPIO_Init();
	MX_USART2_UART_Init();
	BKEL_ADC1_DMA_Init();
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
    SystemCoreClockUpdate();
}

static void BKEL_ADC1_DMA_Init(void)
{
	// PC4,5 = ADC1_IN14, 15
	RCC->APB2ENR |= RCC_IOPC_EN | RCC_ADC1_EN;	// GPIOC, ADC1 Enable
	RCC->AHBENR  |= RCC_DMA1_EN;						// DMA1 Enable

	// PC4, PC5 Analog Mode
	GPIOC->CRL &= ~((GPIO_ANALOG_MODE << GPIO_PC4) |
					(GPIO_ANALOG_MODE << GPIO_PC5));

	/* ADC 클럭 분주: PCLK2 / 6 = 12MHz (최대 14MHz) */
	RCC->CFGR &= ~(RCC_ADC_PRE_CLEAR << RCC_ADC_PRE);
	RCC->CFGR |=  (RCC_ADC_PRE_DIV6 << RCC_ADC_PRE);

	/* DMA1 Channel 1 설정 (ADC1 전용 채널) */
    DMA1_Channel1->CCR &= ~DMA_EN; 			// Disable

    // 채널이 enabled 되어 있는 동안 이 레지스터에 값을 written x
    // This register must not be written when the channel is enabled
    DMA1_Channel1->CPAR  = (uint32_t)ADC1_DR_ADDR;
    DMA1_Channel1->CMAR  = (uint32_t)adc_dma_buf;      	// 메모리 주소
    DMA1_Channel1->CNDTR = ADC_DMA_BUF_LEN;            	// 전송 개수

    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CCR |= DMA_CIRCULAR_EN;  	// CIRC: 원형 버퍼 (무한 루프)
    DMA1_Channel1->CCR |= DMA_MINC_EN;  		// MINC: 메모리 주소 자동 증가
    DMA1_Channel1->CCR |= DMA_PSIZE_16BIT;  		// PSIZE: 16-bit
    DMA1_Channel1->CCR |= DMA_MSIZE_16BIT;		// MSIZE: 16-bit

    DMA1_Channel1->CCR |= DMA_EN;  			// DMA Enable

    /* ADC 기본 설정 */
    ADC1->CR1 = 0;
    ADC1->CR1 |= ADC_SCAN_EN;             // SCAN 모드 활성화

    ADC1->CR2 = 0;
    ADC1->CR2 |= ADC_CONT_EN;  			// CONT: 연속 변환 모드
    ADC1->CR2 |= ADC_DMA_EN;  			// DMA: 변환 결과를 DMA로 전송 활성화

    /* 샘플링 타임 및 채널 순서 설정 (Channel 14, 15) */
    ADC1->SMPR1 &= ~((ADC_SMPR_CLEAR << ADC_SMPR_CH14) |
    				(ADC_SMPR_CLEAR << ADC_SMPR_CH15)); // 채널 14, 15 초기화
    ADC1->SMPR1 |=  (ADC_SMPR_239_5CYC << ADC_SMPR_CH14) |
    				(ADC_SMPR_239_5CYC << ADC_SMPR_CH15); // 239.5 Cycles

    /* 채널 순서 및 개수 설정 (PC4, PC5 읽기) */
    ADC1->SQR1 &= ~ADC_SQ_LEN;
    ADC1->SQR1 |=  ADC_SQ_LEN_2CH;                  // L=1 (2개 채널을 읽음)

    ADC1->SQR3 = 0;
    ADC1->SQR3 |= (ADC_CH14 << ADC_SQ1_POS);                    // 첫 번째 순서: PC4 (채널 14)
    ADC1->SQR3 |= (ADC_CH15 << ADC_SQ2_POS);                    // 두 번째 순서: PC5 (채널 15)

    ADC1->CR2 |= ADC_POWER_ON;      				// ADC ON (power on)
    for (volatile int i = 0; i < 10000; i++);   // 안정화 대기

    // ADC 파워업 시간(안정화 시간)이 지난 후, 소프트웨어에 의해 ADON 비트가 두 번째로 설정될 때 변환이 시작
    /* p218. Conversion starts when ADON bit is set for a second time by software after ADC power-up
    time (tSTAB). */
    ADC1->CR2 |= ADC_POWER_ON;      				// ADC ON (ready)

    ADC1->CR2 |= ADC_RSTCAL_EN;       				// RSTCAL: 캘리브레이션 레지스터 초기화 후 끝날 때까지 대기
    while (ADC1->CR2 & ADC_RSTCAL_EN);

    ADC1->CR2 |= ADC_CAL_EN;       				// CAL: 캘리브레이션 시작. 상태에 맞춰 오차를 자동으로 보정
    while (ADC1->CR2 & ADC_CAL_EN);

    ADC1->CR2 |= ADC_SWSTART_EN;	// SWSTART : 측정 시작
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


static void BKEL_GPIO_Init(void)
{
	RCC->APB2ENR &= ~(0xff << 4);
	RCC->APB2ENR = (GPIOA_RESET | GPIOC_RESET | AFIO_RESET);

	/* PIN MAP */
	/* PA5: GPIO_PIN_LD2 */
	GPIOA->CRL &= ~(BIT_CLEAR << (GPIO_PIN_LD2 * 4));
	GPIOA->CRL |= (BKEL_OUTPUT_PULL << (GPIO_PIN_LD2 * 4));
	/* PC0: GPIO_PIN_INPUT */
	GPIOC->CRL &= ~(BIT_CLEAR << (GPIO_PIN_INPUT * 4));
	GPIOC->CRL |= (BKEL_INPUT_PULL << (GPIO_PIN_INPUT * 4));
	GPIOC->ODR = (1U << 0);		// SET PULL-UP
	/* PC1: GPIO_PIN_OUTPUT */
	GPIOC->CRL &= ~(BIT_CLEAR << (GPIO_PIN_OUTPUT * 4));
	GPIOC->CRL |= (BKEL_OUTPUT_PULL << (GPIO_PIN_OUTPUT * 4));
	/* PC13: GPIO_PIN_B1 */
	GPIOC->CRH &= ~(BIT_CLEAR << (GPIO_PIN_B1 * 4));
	GPIOC->CRH |= (BKEL_INPUT_PULL << (GPIO_PIN_B1 * 4));
	/* PC13: EXTI13 CLEAR 0010: PC[x] pin*/
	AFIO->EXTICR[3] &= ~(BIT_CLEAR << (EXTI_PIN13 * 4));
	AFIO->EXTICR[3] |= (0X2 << 4);
	/*  */
	EXTI->IMR &= ~(1 << 13);
	EXTI->IMR |= (1 << 13);

	EXTI->RTSR &= ~(1 << 13);
	EXTI->RTSR |= (1 << 13);

	NVIC_SetPriority(EXTI15_10_IRQn, 5);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
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
