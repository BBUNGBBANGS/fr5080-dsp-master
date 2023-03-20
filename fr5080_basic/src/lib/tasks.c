#include <stdint.h>

#include "plf.h"
#include "co_list.h"
#include "co_mem.h"

#include "tasks.h"

static struct co_list task_list;

void task_init(void)
{
    co_list_init(&task_list);
}

__attribute__((section("need_kept"))) void task_msg_insert(void *task_msg)
{
    GLOBAL_INT_DISABLE();
    co_list_push_back(&task_list, (struct co_list_hdr *)task_msg);
    GLOBAL_INT_RESTORE();
}

void *task_msg_get(void)
{
    void *msg;

    GLOBAL_INT_DISABLE();
    msg = co_list_pop_front(&task_list);
    GLOBAL_INT_RESTORE();

    return msg;
}

__attribute__((section("need_kept"))) void *task_msg_alloc(uint32_t id, uint32_t msg_size)
{
    struct task_msg_t *msg;

    msg = pvPortMalloc(sizeof(struct task_msg_t) + msg_size);
    msg->id = id;
    msg->param_len = msg_size;

    return msg;
}

void *task_get_handler(uint32_t task_id, const struct task_msg_handler_t *func_table, uint32_t table_size)
{
    for(uint32_t i=0; i<table_size; i++) {
        if((func_table->id == task_id)
            || (func_table->id == TASK_ID_DEFAULT)) {
            return func_table->func;
        }

        func_table++;
    }
    
    return NULL;
}

void task_schedule(const struct task_msg_handler_t *func_table, uint32_t table_size)
{
    struct task_msg_t *msg;
    task_func_t func;
    
    while(1) {
        msg = task_msg_get();
        if(msg) {
            func = task_get_handler(msg->id, func_table, table_size);
            if(func) {
                func(msg);
            }
            vPortFree((void *)msg);
        }
        else {
            //uart_putc_noint('I');
        }
    }
}

