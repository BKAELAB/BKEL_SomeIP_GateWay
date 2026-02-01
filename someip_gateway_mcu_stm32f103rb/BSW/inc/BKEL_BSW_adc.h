/*
 * BKEL_adc.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_BSW_ADC_H_
#define INC_BKEL_BSW_ADC_H_

#include "stm32f1xx_hal.h"

/* ===== ADC 프레임 구성 설정 ===== */
// HT와 TC 인터럽트를 사용하여 2개의 프레임을 교대로 사용
#define ADC_DMA_FRAME     	  2U   // HT / TC -> 2프레임
// 한 프레임에 포함된 채널 수 (CH14, CH15)
#define ADC_CH_PER_FRAME      2U   // CH14, CH15
// 전체 DMA 버퍼 길이 = 2개 프레임 * 2개 채널 = 총 4개의 공간
#define ADC_DMA_BUF_LEN       (ADC_DMA_FRAME * ADC_CH_PER_FRAME)

/* ===== ADC Invalid ===== */
// 데이터가 아직 준비되지 않았을 때 반환할 값
#define ADC_INVALID_VALUE     0xFFFFFFFFUL

/* ===== ADC Channel Pair ===== */
typedef struct
{
    uint16_t ch14;
    uint16_t ch15;
} ADC_ChPair_t;

/* ===== DMA Buffer (extern) ===== */
// 실제 데이터가 저장되는 DMA 전용 버퍼
extern volatile ADC_ChPair_t adc_dma_buf[ADC_DMA_FRAME];

/* ===== ISR Status (extern) ===== */
// 새 데이터가 준비되었음을 알리는 플래그 (1: Ready)
extern volatile uint8_t adc_frame_ready;
// 현재 DMA가 건드리지 않는 안전한 프레임 인덱스 (0 또는 1)
extern volatile uint8_t adc_frame_idx;

uint32_t BKEL_BSW_ADC_GetValue(void);
void getValueTest(void);

#endif /* INC_BKEL_BSW_ADC_H_ */
