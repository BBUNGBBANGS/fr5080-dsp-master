#include "gpio.h"

void gpio_dir(uint8_t bit,bool flag)
{
	if(flag)
		*(volatile uint32_t *)GPIO_DIR |=  (1<<bit);	//set input
	else
		*(volatile uint32_t *)GPIO_DIR &= ~(1<<bit);	//set output
}

void gpio_set(uint8_t bit)
{
	*(volatile uint32_t *)GPIO_DATA |= (1<<bit);
}

void gpio_clear(uint8_t bit)
{
	*(volatile uint32_t *)GPIO_DATA &= ~(1<<bit);
}


uint8_t	read_gpio_bit(uint8_t bit)
{
	return *(volatile uint32_t *)GPIO_DATA & (1<<bit);
}



hal_gpio_fun_t	gpio_driver={

	.gpio_init		=	gpio_dir,
	.gpio_set		=	gpio_set,
	.gpio_clear		=	gpio_clear,
	.gpio_read		=	read_gpio_bit,
};	
