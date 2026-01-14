/*
 * BKEL_BSW_led.c
 *
 *  Created on: Jan 14, 2026
 *      Author: dahyun
 */

#include "BKEL_BSW_led.h"
#include "BKEL_BSW_gpio.h"
#include "BKEL_typedef.h"

static const BKEL_gpio_pin led = {
	.Pin_Channel = LD2_GPIO_Port,
	.Pin_Number  = LD2_Pin
};

void BKEL_LD2_On(void)
{
    BKEL_write_pin(&led, BKEL_GPIO_U_SET);
}

void BKEL_LD2_Off(void)
{
    BKEL_write_pin(&led, BKEL_GPIO_U_RESET);
}

void BKEL_LD2_Toggle(void)
{
    BKEL_toggle_pin(&led);
}
