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

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

void (* volatile user_func_entry)(void) = NULL;

void app_ipc_init(void);

/**
 * This project provides two demo applications:
 * - A simple blinky style demo application.
 * - A more comprehensive test and demo application.
 * The mainCREATE_SIMPLE_BLINKY_DEMO_ONLY macro is used to select between the two.
 *
 * If mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 then the blinky demo will be
 * built. The blinky demo is implemented and described in main_blinky.c.
 *
 * If mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0 then the comprehensive test
 * and demo application will be built. The comprehensive test and demo application
 * is implemented and described in main_full.c.
 */
#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY  0
/*-----------------------------------------------------------*/

/**
 * The entry function for the blinky demo application.
 *
 * This is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1.
 */
extern void main_blinky( void );

/**
 * The entry function for the comprehensive test and demo application.
 *
 * This is used when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
extern void main_full( void );

/**
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.
 *
 * @see http://www.freertos.org/a00016.html
 */
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/**
 * The function called from the tick hook.
 *
 * @note Only the comprehensive demo uses application hook (callback) functions.
 *
 * @see http://www.freertos.org/a00016.html
 */
void vFullDemoTickHookFunction( void );
/*-----------------------------------------------------------*/

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
    qspi_flash_init(4, 1);
    // 与上位机握手，执行烧录，这个参数决定了等待时间，后面可以进行调整，等待时间控制在5ms即可
    app_boot_host_comm(100);

    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_4);

    xthal_set_icacheattr(0x22222244);
    _xtos_set_interrupt_handler(XCHAL_UART_INTERRUPT, uart_isr);
    //task_init();

    printf("\r\nDSP basic function start running.\r\n");

    // 初始化IPC
    app_ipc_init();

    //test_freertos();
    main_full();

    // 收到来自M3的IPC_MSG_EXEC_USER_CODE消息后，这个变量会进行修改，变成非NULL之后就进行调用运行到用户代码，不会再返回
    while(user_func_entry == NULL);
    user_func_entry();

	return 0;
}

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
     * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
     * function that will get called if a call to pvPortMalloc() fails.
     * pvPortMalloc() is called internally by the kernel whenever a task, queue,
     * timer or semaphore is created.  It is also called by various parts of the
     * demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then
     * the size of the  heap available to pvPortMalloc() is defined by
     * configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
     * API function can be used to query the size of free heap space that remains
     * (although it does not provide information on how the remaining heap might be
     * fragmented). See http://www.freertos.org/a00111.html for more information.
     */
    vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected. */
    vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
    /* This function will be called by each tick interrupt if
     * configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
     * added here, but the tick hook is called from an interrupt context, so
     * code must not attempt to block, and only the interrupt safe FreeRTOS API
     * functions can be used (those that end in FromISR()). */
    #if ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 )
    {
        vFullDemoTickHookFunction();
    }
    #endif /* mainCREATE_SIMPLE_BLINKY_DEMO_ONLY */
}
/*-----------------------------------------------------------*/
