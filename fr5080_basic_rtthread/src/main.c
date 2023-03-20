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

#include "rtthread.h"
#include "rtdevice.h"
#include <rthw.h>
#include "serial.h"
#include "lcd.h"

#define TIMER_COUNT         1560000

#define HEAP_MEM_POOL_SIZE              (68*1024)
static uint32_t heap_mem_pool[HEAP_MEM_POOL_SIZE / sizeof(uint32_t)];
void (* volatile user_func_entry)(void) = NULL;

static struct rt_serial_device serial;

void app_ipc_init(void);

void* lodepng_malloc(size_t size)
{
    return rt_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size)
{
    return rt_realloc(ptr, new_size);
}

void lodepng_free(void* ptr)
{
    rt_free(ptr);
}

void rt_hw_console_output(const char *str)
{
    printf(str);
}

void uart_recv_callback(uint8_t c)
{
    rt_hw_serial_isr(&serial, RT_SERIAL_EVENT_RX_IND);
}

static rt_err_t rt_hw_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    return RT_EOK;
}

static rt_err_t rt_hw_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    _xtos_interrupt_enable(XCHAL_UART_INTERRUPT);
    
    return RT_EOK;
}

static int rt_hw_uart_getc(struct rt_serial_device *serial)
{
    uint8_t c;
    int count = 0;

    count = uart_get_data_nodelay_noint(&c, 1);

    if(count)
        return c;
    else
        return -1;
}

static void rt_hw_uart_putc(struct rt_serial_device *serial, uint8_t c)
{
    uart_putc_noint(c);
}

static const struct rt_uart_ops fr508x_uart_ops =
{
    .configure = rt_hw_uart_configure,
    .control = rt_hw_uart_control,
    .putc = rt_hw_uart_putc,
    .getc = rt_hw_uart_getc,
    .dma_transmit = 0
};

static int rt_hw_usart_init(void)
{
    rt_err_t result = 0;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    serial.ops = &fr508x_uart_ops;
    serial.config = config;

    /* register UART device */
    result = rt_hw_serial_register(&serial, RT_CONSOLE_DEVICE_NAME,
                                   RT_DEVICE_FLAG_RDWR|RT_DEVICE_FLAG_INT_RX|RT_DEVICE_FLAG_ACTIVATED,
                                   NULL);
    RT_ASSERT(result == RT_EOK);

    return result;
}

static void rt_hw_timer0_init(void)
{
    uint32_t current_ccount;

    current_ccount = xthal_get_ccount();
    xthal_set_ccompare(XCHAL_TIMER0_INTERRUPT,  current_ccount + TIMER_COUNT);
    _xtos_interrupt_enable(XCHAL_TIMER0_INTERRUPT);
}

void rt_hw_board_init(void)
{
    /* Heap initialization */
#if defined(RT_USING_HEAP)
    rt_system_heap_init((void *)&heap_mem_pool[0], (void *)&heap_mem_pool[HEAP_MEM_POOL_SIZE / sizeof(uint32_t) - 1]);
#endif

    drv_lcd_hw_init();

    /* enable system tick */
    rt_hw_timer0_init();

    xt_set_interrupt_handler(XCHAL_UART_INTERRUPT, uart_isr);
    rt_hw_usart_init();

    /* Set the shell console output device */
#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif
}

/* the system main thread */
void main_thread_entry(void *parameter)
{
#ifdef RT_USING_COMPONENTS_INIT
    /* RT-Thread components initialization */
    rt_components_init();
#endif

    while(1) {
        uint8_t major, minor;
        cpu_usage_get(&major, &minor);
        //printf("main_thread_entry: CPU-USAGE %d.%d\r\n", major, minor);
        rt_thread_mdelay(5000);
    }
}

__attribute__((weak)) void rt_application_init(void)
{
    rt_thread_t tid;

#ifdef RT_USING_HEAP
    tid = rt_thread_create("main", main_thread_entry, RT_NULL,
                           RT_MAIN_THREAD_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(tid != RT_NULL);
#else
    rt_err_t result;

    tid = &main_thread;
    result = rt_thread_init(tid, "main", main_thread_entry, RT_NULL,
                            main_stack, sizeof(main_stack), RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(result == RT_EOK);

    /* if not define RT_USING_HEAP, using to eliminate the warning */
    (void)result;
#endif

    rt_thread_startup(tid);

    cpu_usage_init();
}

int rtthread_startup(void)
{
    rt_hw_interrupt_disable();

    /* board level initialization
     * NOTE: please initialize heap inside board initialization.
     */
    rt_hw_board_init();

    /* show RT-Thread version */
    rt_show_version();

    /* timer system initialization */
    rt_system_timer_init();

    /* scheduler system initialization */
    rt_system_scheduler_init();

#ifdef RT_USING_SIGNALS
    /* signal system initialization */
    rt_system_signal_init();
#endif

    /* create init_thread */
    rt_application_init();

    /* timer thread initialization */
    rt_system_timer_thread_init();

    /* idle thread initialization */
    rt_thread_idle_init();

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&_cpus_lock);
#endif /*RT_USING_SMP*/

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return 0;
}

// 这个工程的代码如无特殊需求，无需修改
int main(void)
{
    *(volatile uint32_t *)(0x50020004) = 0x02;
    *(volatile uint32_t *)(0x50020000) = 0xff;

    // 初始化串口用于程序烧录
    uart_init(UART_HW_BAUD_RATE_115200, uart_recv_callback);

    // 初始化qspi，用于支持flash烧录和XIP
    qspi_flash_init(4, 1);
    // 与上位机握手，执行烧录，这个参数决定了等待时间，后面可以进行调整，等待时间控制在5ms即可
    app_boot_host_comm(100);

    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_32);

    xthal_set_icacheattr(0x22222244);

    printf("\r\nDSP basic function start running.\r\n");

    // 初始化IPC
    //app_ipc_init();

    rtthread_startup();

    // 收到来自M3的IPC_MSG_EXEC_USER_CODE消息后，这个变量会进行修改，变成非NULL之后就进行调用运行到用户代码，不会再返回
    while(user_func_entry == NULL);
    user_func_entry();

    return 0;
}

