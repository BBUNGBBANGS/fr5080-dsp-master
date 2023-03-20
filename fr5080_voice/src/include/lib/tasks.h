#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>

#include "co_list.h"

#define TASK_MSG_HANDLER_TABLE_SIZE(x)      (sizeof(x)/sizeof(struct task_msg_handler_t))

enum task_id_t {
    TASK_ID_DEFAULT = 0xffffffff,
};

enum task_msg_id_t {
    AUDIO_IPC_DMA_RX,
    AUDIO_IPC_DMA_TX,
    AUDIO_ALGO_CREATE,
    AUDIO_ALGO_DESTROY,
};

struct task_msg_t {
    struct co_list_hdr hdr;
    uint32_t id;
    uint32_t param_len;
    uint32_t param[0];
};

typedef void (*task_func_t)(struct task_msg_t *msg);

struct task_msg_handler_t {
    uint32_t id;
    task_func_t func;
};

void task_init(void);
void task_msg_insert(void *task_msg);
void *task_msg_alloc(uint32_t id, uint32_t msg_size);
void task_schedule(const struct task_msg_handler_t *func_table, uint32_t table_size);

#endif  // _TASK_H

