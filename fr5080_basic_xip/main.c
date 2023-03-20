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

// ������̵Ĵ��������������������޸�
int main(void)
{
    *(volatile uint32_t *)(0x50020004) = 0x02;
    *(volatile uint32_t *)(0x50020000) = 0xff;

    // ��ʼ���������ڳ�����¼
    uart_init(BAUD_RATE_115200, uart_recv_callback);

    // ��ʼ��qspi������֧��flash��¼��XIP
    qspi_flash_init(1, 1);
    // ����λ�����֣�ִ����¼��������������˵ȴ�ʱ�䣬������Խ��е������ȴ�ʱ�������5ms����
    app_boot_host_comm(100);

    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);

    xthal_set_icacheattr(0x22222244);
    _xtos_set_interrupt_handler(XCHAL_UART_INTERRUPT, uart_isr);
    //task_init();

    printf("\r\nDSP basic function start running.\r\n");

    // ��ʼ��IPC
    app_ipc_init();

    // �յ�����M3��IPC_MSG_EXEC_USER_CODE��Ϣ���������������޸ģ���ɷ�NULL֮��ͽ��е������е��û����룬�����ٷ���
    while(user_func_entry == NULL);
    user_func_entry();

	return 0;
}

