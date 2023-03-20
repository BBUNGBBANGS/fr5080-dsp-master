/*
 * os_timer.h
 *
 *  Created on: 2018-12-17
 *      Author: Administrator
 */

#ifndef _OS_TIMER_H
#define _OS_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#include "co_list.h"

typedef void (*os_timer_func_t)(void *arg);

enum os_timer_type_t {
	OS_TIMER_TYPE_SINGLE,
	OS_TIMER_TYPE_REPEAT,
};

struct os_timer_t {
	struct os_timer_t *next;

	os_timer_func_t callback;
	void *arg;

	uint32_t last_period;
	uint32_t initial_period;
	bool running;
	enum os_timer_type_t type;
};

struct os_timer_msg_t {
	struct os_timer_t *timer;
};

void os_timer_engine_init(void);

void os_timer_init(struct os_timer_t *timer, os_timer_func_t callback, void *parg);

void os_timer_destroy(struct os_timer_t *ptimer);

void os_timer_start(struct os_timer_t *timer, uint32_t period_ms, enum os_timer_type_t type);

void os_timer_stop(struct os_timer_t * timer);

#endif /* _OS_TIMER_H */
