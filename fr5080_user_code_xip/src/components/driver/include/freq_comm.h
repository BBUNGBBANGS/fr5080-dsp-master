#ifndef	__FREQ_COMM_H
#define __FREQ_COMM_H

#include <stdbool.h>
#include <stdint.h>

#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>
#include "gpio.h"
#include "qspi.h"
#include "uart.h"
#include "plf.h"


//DIR 0:out 1:input
#define GPIO_DATA           (SYSTEM_BASE+0)
#define GPIO_DIR            (SYSTEM_BASE+4)


#endif	/*	__FREQ_COMM_H	*/








