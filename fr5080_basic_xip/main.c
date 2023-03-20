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
#include "qspi.h"
#include "flash.h"

void (* volatile user_func_entry)(void) = NULL;

void app_ipc_init(void);

void uart_recv_callback(uint8_t c)
{
    uart_putc_noint(c);
}

// 这个工程的代码如无特殊需求，无需修改
int main(void)
{
    *(volatile uint32_t *)(0x50020004) = 0x02;
    *(volatile uint32_t *)(0x50020000) = 0xff;

    // 初始化串口用于程序烧录
    uart_init(BAUD_RATE_115200, uart_recv_callback);

    // 初始化qspi，用于支持flash烧录和XIP
    qspi_flash_init(1, 1);
    // 与上位机握手，执行烧录，这个参数决定了等待时间，后面可以进行调整，等待时间控制在5ms即可
    app_boot_host_comm(100);

    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);

    xthal_set_icacheattr(0x22222244);
    _xtos_set_interrupt_handler(XCHAL_UART_INTERRUPT, uart_isr);
    //task_init();

    printf("\r\nDSP basic function start running.\r\n");

    // 初始化IPC
    app_ipc_init();

    // 收到来自M3的IPC_MSG_EXEC_USER_CODE消息后，这个变量会进行修改，变成非NULL之后就进行调用运行到用户代码，不会再返回
    while(user_func_entry == NULL);
    user_func_entry();

	return 0;
}

