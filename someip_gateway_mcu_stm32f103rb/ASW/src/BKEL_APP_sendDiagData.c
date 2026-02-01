/*
 * BKEL_APP_sendDiagData.c
 *
 *  Created on: Jan 18, 2026
 *      Author: PNYcom
 */

#include <BKEL_APP_sendDiagData.h>
#include <BKEL_BSW_pwm.h>
#include <BKEL_BSW_uart.h>
#include <BKEL_BSW_adc.h>
#include <main.h>
#include <string.h>

void AppSendDiagPWMOut()
{
	uint8_t out_buf[64];

	uint8_t readPWM = BKEL_PWM_ReadDuty();
	uint8_t readPeriod = BKEL_PWM_ReadPeriod();
	uint8_t payload[2];

	payload[0] = readPWM;
	payload[1] = readPeriod;

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x20, (uint8_t)0x02, payload, (uint16_t)2U);

	BKEL_UART_Tx(out_buf, frame_len);
}

void AppSendDiagPWMIn()
{
	uint8_t out_buf[64];
	BKEL_BSW_PwmInput_t pwm;

	BKEL_PWM_Input_Read_TIM3(&pwm);
	if (pwm.valid)
	{
		uint8_t readPWM = pwm.duty_percent;
		uint8_t readPeriod = (pwm.freq_hz/999U);
		uint8_t payload[2];
		payload[0] = readPWM;
		payload[1] = readPeriod;

		size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x21, (uint8_t)0x02, payload, (uint16_t)2U);
		BKEL_UART_Tx(out_buf, frame_len);
	}
	else
	{
		return; // Error
	}
}

void AppSendDiagADC1Val()
{
	uint8_t out_buf[64];
	uint32_t u32AdcVal = BKEL_BSW_ADC_GetValue();
	uint8_t payload[2];
	uint16_t u16AdcVal = ((u32AdcVal >> 16U) & 0xFFFF);
	memcpy(payload, &u16AdcVal, 2U);

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x22, (uint8_t)0x02, payload, (uint16_t)2U);
	BKEL_UART_Tx(out_buf, frame_len);
}

void AppSendDiagADC2Val()
{
	uint8_t out_buf[64];
	uint32_t u32AdcVal = BKEL_BSW_ADC_GetValue();
	uint8_t payload[2];
	uint16_t u16AdcVal = ((u32AdcVal) & 0xFFFF);
	memcpy(payload, &u16AdcVal, 2U);

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x22, (uint8_t)0x02, payload, (uint16_t)2U);
	BKEL_UART_Tx(out_buf, frame_len);
}

void AppSendDiagGPOPinState()
{
	uint8_t out_buf[64];
	uint8_t payload[1];
	BKEL_gpio_pin tGPOPin = {
			GPIOC, 1U
	};
	payload[0] = BKEL_read_pin(&tGPOPin);

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x23, (uint8_t)0x01, payload, (uint16_t)1U);
	BKEL_UART_Tx(out_buf, frame_len);
}

void AppSendDiagGPIPinState()
{
	uint8_t out_buf[64];
	uint8_t payload[1];
	BKEL_gpio_pin tGPIPin = {
			GPIOC, 0U
	};
	payload[0] = BKEL_read_pin(&tGPIPin);

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x23, (uint8_t)0x01, payload, (uint16_t)1U);
	BKEL_UART_Tx(out_buf, frame_len);
}

void AppSendDiagLD2PinState()
{
	uint8_t out_buf[64];
	uint8_t payload[1];
	BKEL_gpio_pin tLD2Pin = {
			GPIOA, 5U
	};
	payload[0] = BKEL_read_pin(&tLD2Pin);

	size_t frame_len = build_frame(out_buf, 64U, (uint8_t)0x23, (uint8_t)0x01, payload, (uint16_t)1U);
	BKEL_UART_Tx(out_buf, frame_len);
}
