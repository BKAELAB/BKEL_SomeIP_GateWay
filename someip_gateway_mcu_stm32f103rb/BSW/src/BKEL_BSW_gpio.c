#include <BKEL_BSW_gpio.h>

BKEL_GPIO_STATE_T BKEL_read_pin(BKEL_gpio_pin * gpiopin)
{
	GPIO_TypeDef* gpio_channel = gpiopin->Pin_Channel;
	uint16_t 	  gpio_number = gpiopin->Pin_Number;

	BKEL_GPIO_STATE_T retGpioState;

	if ((gpio_channel->IDR & gpio_number) != (uint32_t)BKEL_GPIO_U_SET)
	{
		retGpioState = BKEL_GPIO_U_RESET;
	}
	else
	{
		retGpioState = BKEL_GPIO_U_SET;
	}

	return retGpioState;
}
