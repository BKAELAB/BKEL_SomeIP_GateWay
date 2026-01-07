/*
 * BKEL_adc.c
 *
 *  Created on: Dec 20, 2025
 *      Author: PNYcom
 */

#include <BKEL_BSW_adc.h>

volatile uint16_t adc_pc4[ADC_DMA_BUF_LEN / 2];
volatile uint16_t adc_pc5[ADC_DMA_BUF_LEN / 2];

uint32_t BKEL_BSW_ADC_GetValue(void)
{
	static uint8_t hasData = 0U;
	// 남은 전송 횟수: 16부터 줄어듦
	uint32_t cndtr = DMA1_Channel1->CNDTR;

	if ((cndtr == ADC_DMA_BUF_LEN) && (hasData == 0U)) { return ADC_INVALID_VALUE; } // 아직 값이 없음

	// 버퍼가 0이 아님
	if (cndtr != ADC_DMA_BUF_LEN){ hasData = 1U; }

	// 다음버퍼에 쓸 위치, 원형큐이므로 modulo
	uint32_t writePos = (ADC_DMA_BUF_LEN - cndtr) % ADC_DMA_BUF_LEN;

	// ch14, 15 pair 로 반환하기 위해 -ADC_CH_COUNT
	uint32_t lastIdx = (writePos + ADC_DMA_BUF_LEN - ADC_CH_COUNT) % ADC_DMA_BUF_LEN;
	// lastIdx = lastIdx % ADC_CH_COUNT;
	lastIdx &= ~1U; // 채널 2개가 한 쌍이므로 LSB 강제 0 변환

	uint16_t ch14 = adc_dma_buf[lastIdx];
	uint16_t ch15 = adc_dma_buf[lastIdx+1];

	//vol1. 채널14, 15 동시 반환
	return ((uint32_t)ch14 << 16U) | ch15;

	//vol2. 그냥 최신 채널 하나 반환
	//return (uint32_t)adc_dma_buf[lastIdx];
}
/* ADC getValue test */
void getValueTest(void)
{
	uint32_t getVal = BKEL_BSW_ADC_GetValue();
	printf("ADC_ch14: %u, ADC_ch15: %u\r\n",
				    (uint16_t)(getVal >> 16U),
					(uint16_t)(getVal & 0xFFFFU));
}
