/*
 * BKEL_adc.c
 *
 *  Created on: Dec 20, 2025
 *      Author: PNYcom
 */

#include <BKEL_BSW_adc.h>

/* ===== DMA Buffer ===== */
volatile ADC_ChPair_t adc_dma_buf[ADC_DMA_FRAME];

/* ===== ISR Flags ===== */
volatile uint8_t adc_frame_ready = 0;
volatile uint8_t adc_frame_idx   = 0;


/* Thread-safe ADC GetValue */
// [31:16] CH14 값, [15:0] CH15 값. 데이터 없으면 INVALID_VALUE 반환
uint32_t BKEL_BSW_ADC_GetValue(void)
{
    static ADC_ChPair_t shadow;
    static uint8_t valid = 0;

    // ISR(인터럽트)에 의해 새 데이터 프레임이 완성되었는지 확인
    if (adc_frame_ready)
    {
        uint8_t idx;

        // 복사 도중에 인덱스가 바뀌는 것을 방지
        __disable_irq();
        idx = adc_frame_idx;
        shadow = adc_dma_buf[idx];	// DMA 영역에서 소프트웨어 영역(shadow)으로 데이터 복사
        adc_frame_ready = 0;		// 읽기 완료했으므로 플래그 초기화
        valid = 1;
        __enable_irq();
    }

    // 만약 한 번도 데이터가 들어온 적 없다면 에러 반환
    if (!valid)
    {
        return ADC_INVALID_VALUE;
    }

    /* CH14를 상위 16비트로, CH15를 하위 16비트로 합쳐서 반환 */
    return ((uint32_t)shadow.ch14 << 16U) | shadow.ch15;
}
