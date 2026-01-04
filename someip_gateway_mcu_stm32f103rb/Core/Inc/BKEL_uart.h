// 26.01.04 Panho //////////////////////////////////////////////////////
#ifndef __BKEL_UART_H__
#define __BKEL_UART_H__

/* Base */
#define PAN_RCC           			0x40021000
#define PAN_GPIOA         			0x40010800
#define PAN_USART2        			0x40004400
#define PAN_DMA1         			0x40020000
#define PAN_NVIC_ISER1         		(*((volatile unsigned int*)0xE000E104))

/* RCC */
#define PAN_RCC_APB1ENR       		(*((volatile unsigned int*)(PAN_RCC + 0x1C)))
#define PAN_RCC_AHBENR        		(*((volatile unsigned int*)(PAN_RCC + 0x14)))

/* GPIOA */
#define PAN_GPIOA_CRL         		(*((volatile unsigned int*)(PAN_GPIOA + 0x00)))

/* USART2 */
#define PAN_USART2_SR         		(*((volatile unsigned int*)(PAN_USART2 + 0x00)))
#define PAN_USART2_DR         		(*((volatile unsigned int*)(PAN_USART2 + 0x04)))
#define PAN_USART2_BRR        		(*((volatile unsigned int*)(PAN_USART2 + 0x08)))
#define PAN_USART2_CR1        		(*((volatile unsigned int*)(PAN_USART2 + 0x0C)))
#define PAN_USART2_CR3        		(*((volatile unsigned int*)(PAN_USART2 + 0x14)))

/* DMA1 */
#define PAN_DMA1_CCR      			(*((volatile unsigned int*)(PAN_DMA1 + 0x6C)))
#define PAN_DMA1_CNDTR    			(*((volatile unsigned int*)(PAN_DMA1 + 0x70)))
#define PAN_DMA1_CPAR     			(*((volatile unsigned int*)(PAN_DMA1 + 0x74)))
#define PAN_DMA1_CMAR     			(*((volatile unsigned int*)(PAN_DMA1 + 0x78)))

/* 연산용 */
#define PAN_USART_CR1_UE          	(0x01 << 13) 	/* USART enable */
#define PAN_USART_CR1_M           	(0x01 << 12)	/* Word length */
#define PAN_USART_CR1_PCE         	(0x01 << 10)	/* Parity control enable */
#define PAN_USART_CR1_IDLEIE    	(0x01 << 4)			/* IDLE interrupt enable */
#define PAN_USART_CR1_TE          	(0x01 << 3)		/* Transmitter enable */
#define PAN_USART_CR1_RE          	(0x01 << 2)		/* Receiver enable */
#define PAN_USART_CR3_DMAR        	(0x01 << 6)		/* DMA enable receiver */

// 버퍼
#define UART_RX_BUF_SIZE    		256
extern volatile uint8_t uart_rx_dma_buf[UART_RX_BUF_SIZE]; // DMA rx 버퍼용

#endif // __BKEL_UART_H__
////////////////////////////////////////////////////////////////////////
