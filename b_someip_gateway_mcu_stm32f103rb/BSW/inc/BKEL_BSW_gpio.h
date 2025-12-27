/*
 * BKEL_adc.h
 *
 *  Created on: Dec 20, 2025
 *      Author: seokjun.kang
 */

#ifndef BSW_INC_BKEL_GPIO_H
#define BSW_INC_BKEL_GPIO_H

#include "main.h"

typedef enum BKEL_GPIO_STATE {
	BKEL_GPIO_U_SET,
	BKEL_GPIO_U_RESET,
	BKEL_GPIO_U_STATE_END,
} BKEL_GPIO_STATE_T;

typedef struct BKEL_GPIO {
	GPIO_TypeDef* Pin_Channel;
	uint16_t	  Pin_Number;
} BKEL_gpio_pin;

// Functions Prototype
BKEL_GPIO_STATE_T BKEL_read_pin(BKEL_gpio_pin * gpiopin);


#endif	// BSW_INC_BKEL_GPIO_H
