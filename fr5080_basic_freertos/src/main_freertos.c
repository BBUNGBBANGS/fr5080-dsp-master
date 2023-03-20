/*
 * main_freertos.c
 *
 *  Created on: 2018-11-24
 *      Author: owen
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/**
 * Priorities at which the tasks are created.
 */
#define mainCHECK_TASK_PRIORITY         ( configMAX_PRIORITIES - 2 )

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    printf("ASSERT: %s, %d\r\n", pcFileName, ulLine);
}

void test_freertos_task(void)
{
    // Block for 1000ms.
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;

    while(1) {
        printf("test_freertos_task is running.\r\n");
        vTaskDelay(xDelay);
    }
}

void test_freertos(struct task_msg_t *msg)
{
    /* Start the check task as described at the top of this file. */
    xTaskCreate( test_freertos_task, "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );

    /* Start the scheduler itself. */
    vTaskStartScheduler();
}

