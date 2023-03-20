/*
 * main.c
 *
 *  Created on: 2018-7-3
 *      Author: Administrator
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "plf.h"
#include "ipc.h"
#include "uart.h"
#include "tasks.h"

void app_ipc_init(void);

void uart_recv_callback(uint8_t c)
{
    uart_putc_noint(c);
}

int main(void)
{
    xthal_set_icacheattr(0x22222244);
    uart_init(BAUD_RATE_921600, uart_recv_callback);
    _xtos_set_interrupt_handler(XCHAL_UART_INTERRUPT, uart_isr);
    task_init();

    printf("DSP basic function start running.\r\n");

    app_ipc_init();

	return 0;
}

