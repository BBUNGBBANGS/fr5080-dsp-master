/*
 * os_timer.c
 *
 *  Created on: 2018-12-17
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>

#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "co_list.h"
#include "tasks.h"
#include "plf.h"

#include "os_timer.h"

#define OS_TIMER_DEBUG				printf

#define system_clock                156
#define FR5080H_1ms_COUNT         	(system_clock * 1000)
#define OS_TIMER_PRECISION			1	// unit: ms
#define OS_TIMER_PRECISION_COUNTER	(OS_TIMER_PRECISION * FR5080H_1ms_COUNT)

static struct os_timer_t *timer_list = NULL;

static uint8_t is_timer_in_list(struct os_timer_t *ptimer)
{
    struct os_timer_t *curr=NULL;

    for (curr = timer_list;
         curr;
         curr = curr->next)
    {
        if ((void *)curr == (void *)ptimer)
        {
            return 1;
        }
    }
    return 0;
}

void os_timer_init(struct os_timer_t *timer, os_timer_func_t callback, void *parg)
{
	GLOBAL_INT_DISABLE();
	if(is_timer_in_list(timer) == 0) {
		timer->callback = callback;
		timer->arg = parg;
		timer->running = false;
		timer->next = timer_list;
		timer_list = timer;
		OS_TIMER_DEBUG("os_timer_init: %08x.\r\n", timer);
	}
	GLOBAL_INT_RESTORE();
}

void os_timer_destroy(struct os_timer_t *ptimer)
{
	GLOBAL_INT_DISABLE();
	struct os_timer_t *curr, *prev;

	for (curr = timer_list, prev = NULL;
		 curr;
		 curr = curr->next) {
		if(curr == ptimer) {
			if(prev) {
				prev->next = curr->next;
			}
			else {
				timer_list = timer_list->next;
			}
			break;
		}
		prev = curr;
	}
	GLOBAL_INT_RESTORE();
}

void os_timer_start(struct os_timer_t *timer, uint32_t period_ms, enum os_timer_type_t type)
{
	GLOBAL_INT_DISABLE();
	if(is_timer_in_list(timer)) {
		period_ms /= OS_TIMER_PRECISION;
		if(period_ms == 0) {
			period_ms = 1;
		}
		timer->initial_period = period_ms;
		timer->last_period = period_ms;
		timer->type = type;
		timer->running = true;

		OS_TIMER_DEBUG("os_timer_start: %08x.\r\n", timer);
	}
	GLOBAL_INT_RESTORE();
}

void os_timer_stop(struct os_timer_t *timer)
{
	GLOBAL_INT_DISABLE();
	timer->running = false;
	GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) static void os_timer_check_list(void)
{
	struct os_timer_t *next, *curr;

	GLOBAL_INT_DISABLE();
	for (curr = timer_list; curr; ) {
		next = curr->next;
		if(curr->running) {
			curr->last_period--;
			if(curr->last_period == 0) {
				// 创建一个新的消息，并推送到消息列表
				struct task_msg_t *msg = task_msg_alloc(OS_TIMER_TRIGGER, sizeof(struct os_timer_msg_t));
				struct os_timer_msg_t *os_timer_msg;
				os_timer_msg = (struct os_timer_msg_t *)msg->param;
				os_timer_msg->timer = curr;
				task_msg_insert(msg);

				if(curr->type == OS_TIMER_TYPE_REPEAT) {
					curr->last_period = curr->initial_period;
					curr->running = true;
				}
				else {
					curr->running = false;
				}
			}
		}
		curr = next;
	}
	GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) static void timer1_isr(void)
{
	uint32_t begin_count;
	os_timer_check_list();
	begin_count = xthal_get_ccount();
	xthal_set_ccompare(XCHAL_TIMER1, begin_count + OS_TIMER_PRECISION_COUNTER);

	//lv_tick_inc(OS_TIMER_PRECISION);
}

__attribute__((section("iram_section"))) void os_timer_trigger_handler(struct task_msg_t *msg)
{
	struct os_timer_msg_t *timer_msg = (void *)msg->param;
	struct os_timer_t *timer;

	timer = timer_msg->timer;

	timer->callback(timer->arg);
}

void os_timer_engine_init(void)
{
	uint32_t begin_count;

	_xtos_set_interrupt_handler(XCHAL_TIMER1, timer1_isr);
	begin_count = xthal_get_ccount();
	xthal_set_ccompare(XCHAL_TIMER1, begin_count + OS_TIMER_PRECISION_COUNTER);

	_xtos_interrupt_enable(XCHAL_TIMER1);
}
