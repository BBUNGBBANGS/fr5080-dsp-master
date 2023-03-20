#ifndef		__GPIO_H
#define 	__GPIO_H
		  
#include "freq_comm.h"
#include "hal_gpio.h"

typedef enum
{
    GPIO_BIT_0,
    GPIO_BIT_1,
    GPIO_BIT_2,
    GPIO_BIT_3,
    GPIO_BIT_4,
    GPIO_BIT_5,
    GPIO_BIT_6,
    GPIO_BIT_7,
} system_port_bit_t;

enum {
	GPIO_OUT,
	GPIO_INPUT,
};


#endif	/*	__GPIO_H	*/


