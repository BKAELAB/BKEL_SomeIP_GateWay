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
	static uint32_t prev_cndtr = ADC_DMA_BUF_LEN;
	// 남은 전송 횟수: 16부터 줄어듦
	uint32_t cndtr = DMA1_Channel1->CNDTR;

	if ((cndtr != ADC_DMA_BUF_LEN)) // 아직 값이 없음
	{
		hasData = 1U;
	}
	else
	{
		// 1. 아직 시작 전인 경우
		// 2. 한 바퀴 돌아서 리로드 순간인 경우
		// prev_cndtr 로 과거에 데이터 있었는지 확인
		if ((hasData == 0U) && (prev_cndtr == ADC_DMA_BUF_LEN))
		{
			return ADC_INVALID_VALUE;
		}
	}
	// cndtr 값 갱신
	cndtr = DMA1_Channel1->CNDTR;

	// 다음버퍼에 쓸 위치, 원형큐이므로 modulo
	uint32_t writePos = (ADC_DMA_BUF_LEN - cndtr) % ADC_DMA_BUF_LEN;

	// ch14, 15 pair 로 반환하기 위해 -ADC_CH_COUNT
	uint32_t lastIdx = (writePos + ADC_DMA_BUF_LEN - ADC_CH_COUNT) % ADC_DMA_BUF_LEN;
	// lastIdx = lastIdx % ADC_CH_COUNT;
	lastIdx &= ~1U; // 채널 2개가 한 쌍이므로 LSB 강제 0 변환

	/*
	// 최신 데이터를 받아오기 위해
	uint32_t cndtr2 = DMA1_Channel1->CNDTR;

	if (cndtr != cndtr2)
	{
		return ADC_INVALID_VALUE;
	}
	*/
	uint16_t ch14 = adc_dma_buf[lastIdx];
	uint16_t ch15 = adc_dma_buf[lastIdx+1];

	// 이전 cndtr 저장, 한번 돌아간 적이 있는가
	prev_cndtr = cndtr;

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
