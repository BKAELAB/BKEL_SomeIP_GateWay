#include <BKEL_BSW_gpio.h>

BKEL_GPIO_STATE_T BKEL_read_pin(BKEL_gpio_pin * gpiopin)
{
	GPIO_TypeDef* gpio_channel = gpiopin->Pin_Channel;
	uint16_t 	  gpio_number = gpiopin->Pin_Number;

	BKEL_GPIO_STATE_T retGpioState;

	if ((gpio_channel->IDR & gpio_number) != (uint32_t)BKEL_GPIO_U_RESET)
	{
		retGpioState = BKEL_GPIO_U_SET;
	}
	else
	{
		retGpioState = BKEL_GPIO_U_RESET;
	}

	return retGpioState;
}

void BKEL_write_pin(BKEL_gpio_pin * gpiopin, BKEL_GPIO_STATE_T pinstate)
{
	GPIO_TypeDef* gpio_channel = gpiopin->Pin_Channel;
	uint16_t 	  gpio_number = gpiopin->Pin_Number;
	// RESET: 0 SET: 1
	// BSRR[31:16] RESET
	// BSRR[15:0] SET
	if(pinstate != BKEL_GPIO_U_RESET)
	{
		gpio_channel->BSRR = gpio_number;
	}
	else
	{
		gpio_channel->BSRR = (uint32_t)gpio_number << 16U;
	}

}

void BKEL_toggle_pin(BKEL_gpio_pin * gpiopin)
{
	GPIO_TypeDef* gpio_channel = gpiopin->Pin_Channel;
	uint16_t 	  gpio_number = gpiopin->Pin_Number;

	// 현재 핀 상태 받아옴
	uint32_t odr;

	odr = gpio_channel->ODR;
//ver1
//	if((odr & gpio_number) != BKEL_GPIO_U_SET) // 1이면, 0
//	{
//		gpio_channel->BSRR = odr << 16U;
//	}
//	else // 0 이면 1
//	{
//		gpio_channel->BSRR = ~odr & gpio_number;
//	}

	//ver2
	gpio_channel->BSRR = (odr & gpio_number) << 16U | (~odr & gpio_number);
}




