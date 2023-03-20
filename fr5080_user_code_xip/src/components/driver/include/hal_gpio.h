#ifndef		__HAL_GPIO_H
#define 	__HAL_GPIO_H

#include "freq_comm.h"

typedef	struct {

	void	(*gpio_init)	(uint8_t bit,bool flag);
	void	(*gpio_set)		(uint8_t bit);
	void	(*gpio_clear)	(uint8_t bit);
	uint8_t (*gpio_read)	(uint8_t bit);
}hal_gpio_fun_t;



void	hal_gpio_init(uint8_t bit,uint8_t dir);
void	hal_gpio_set(uint8_t bit);
void	hal_gpio_clear(uint8_t bit);
uint8_t	hal_gpio_read(uint8_t bit);



#endif	/*	__HAL_GPIO_H	*/













