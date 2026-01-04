/*
 * BKEL_BSW_spi.c
 *
 *  Created on: Jan 4, 2026
 *      Author: bazzi
 */

#include "BKEL_BSW_spi.h"
#include <stdio.h>

/* 송수신 함수 */
uint8_t BKEL_SPI2_Transfer(uint8_t data) {
    uint32_t timeout = 10000;
    /* 송신 버퍼가 비어있는지 확인 (TXE) */
    while (!(SPI2->SR & SPI_SR_TXE)) {
        if (--timeout == 0) return 0;	// 송신 실패 시 탈출
    }
    /* 데이터를 데이터 레지스터(DR)에 씀 (전송 시작) */
    SPI2->DR = data;

    timeout = 10000;					// 타임아웃 초기화
    while (!(SPI2->SR & SPI_SR_RXNE)) {
        if (--timeout == 0) return 0;	// 수신 실패 시 탈출
    }

    /* 수신된 데이터 리턴 */
    //데이터를 쓸 때, 읽을 때 둘 다 SPI2->DR 사용
    return (uint8_t)SPI2->DR;
}


/* SPI loopback test */
void BKEL_SPI2_Loopback(void) {
	static uint32_t transfer_count = 0;

	static uint8_t txValue = 0x01; // 보낼 데이터
	static uint8_t rxValue = 0x01;
	rxValue = BKEL_SPI2_Transfer(txValue++); // 받을 데이터

	transfer_count++;
	printf("[SPI TEST #%lu] TX: 0x%02X, RX: 0x%02X\r\n", transfer_count, txValue, rxValue);

	if(txValue == rxValue) {
		GPIOA->BSRR = (1U << 5); // LED ON
		vTaskDelay(1000);
		GPIOA->BRR = (1U << 5); // LED OFF
		vTaskDelay(1000);
		GPIOA->BSRR = (1U << 5); // LED ON
	}
	else {
		GPIOA->BRR = (1U << 5);  // LED OFF
	}
}
