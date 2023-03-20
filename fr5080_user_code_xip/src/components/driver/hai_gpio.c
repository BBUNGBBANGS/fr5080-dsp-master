#include "hal_gpio.h"

extern hal_gpio_fun_t	gpio_driver;
hal_gpio_fun_t	*gpio_dev	=	&gpio_driver;

void hal_gpio_init(uint8_t bit,uint8_t dir)
{
	gpio_dev->gpio_init(bit,dir);
}

void hal_gpio_set(uint8_t bit)
{
	gpio_dev->gpio_set(bit);
}

void hal_gpio_clear(uint8_t bit)
{
	gpio_dev->gpio_clear(bit);
}

uint8_t	hal_gpio_read(uint8_t bit)
{
	return gpio_dev->gpio_read(bit);
}
