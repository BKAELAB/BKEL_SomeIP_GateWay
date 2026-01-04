/*
 * BKEL_sysinit.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef INC_BKEL_SYSINIT_H_
#define INC_BKEL_SYSINIT_H_

#include "stream_buffer.h"
#include "FreeRTOS.h"

extern StreamBufferHandle_t xStreamBuffer;

/* function prototypes -----------------------------------------------*/

void system_init(void);

#endif /* INC_BKEL_SYSINIT_H_ */
