#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>

#include "co_list.h"

#define TASK_MSG_HANDLER_TABLE_SIZE(x)      (sizeof(x)/sizeof(struct task_msg_handler_t))

enum task_id_t {
    TASK_ID_DEFAULT = 0xffffffff,
};

enum task_msg_id_t {
    OS_TIMER_TRIGGER,
    MP3_DECODER_INIT,
    MP3_DECODER_DO_DECODE,
    MP3_DECODER_TRANSMIT_NEXT_PCM,
    MCU_NEED_MORE_SBC_DATA,
    SBC_ENCODER_TRANSMIT_NEXT_FRAME,
    AUDIO_IPC_DMA_RX,
    AUDIO_IPC_DMA_TX,
    TEST_FLASH_READ_WRITE,
    AUDIO_ALGO_CREATE,
    AUDIO_ALGO_DESTROY,
    DECODER_PREPARE_FOR_NEXT,
    RECEIVE_AT_COMMAND,
    SBC_DECODER_DO_DECODE,
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
void task_msg_insert_front(void *task_msg);
void *task_msg_alloc(uint32_t id, uint32_t msg_size);
void task_schedule(const struct task_msg_handler_t *func_table, uint32_t table_size);

#endif  // _TASK_H

