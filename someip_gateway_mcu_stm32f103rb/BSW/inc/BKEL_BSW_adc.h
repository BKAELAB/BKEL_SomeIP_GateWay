/*
 * BKEL_adc.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_BSW_ADC_H_
#define INC_BKEL_BSW_ADC_H_

#include "stm32f1xx_hal.h"

/* ===== ADC Frame Configuration ===== */
#define ADC_DMA_FRAME     	  2U   // HT / TC -> 2프레임
#define ADC_CH_PER_FRAME      2U   // CH14, CH15
#define ADC_DMA_BUF_LEN       (ADC_DMA_FRAME * ADC_CH_PER_FRAME)

/* ===== ADC Invalid ===== */
#define ADC_INVALID_VALUE     0xFFFFFFFFUL

/* ===== ADC Channel Pair ===== */
typedef struct
{
    uint16_t ch14;
    uint16_t ch15;
} ADC_ChPair_t;

/* ===== DMA Buffer (extern) ===== */
extern volatile ADC_ChPair_t adc_dma_buf[ADC_DMA_FRAME];

/* ===== ISR Status (extern) ===== */
extern volatile uint8_t adc_frame_ready;
extern volatile uint8_t adc_frame_idx;


//#define ADC_DMA_BUF_LEN  16U
//#define ADC_CH_COUNT	 2U			// 채널2개 ch14, ch15
//#define ADC_INVALID_VALUE	(UINT32_MAX)
//extern volatile uint16_t adc_dma_buf[ADC_DMA_BUF_LEN];
//extern volatile uint16_t adc_pc4[ADC_DMA_BUF_LEN / 2];
//extern volatile uint16_t adc_pc5[ADC_DMA_BUF_LEN / 2];

uint32_t BKEL_BSW_ADC_GetValue(void);
void getValueTest(void);

#endif /* INC_BKEL_BSW_ADC_H_ */
