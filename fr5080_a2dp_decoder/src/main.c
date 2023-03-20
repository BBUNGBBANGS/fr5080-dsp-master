/*
 * main.c
 *
 *  Created on: 2018-8-19
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>

#include "ipc.h"
#include "user_def.h"
#include "plf.h"
#include "flash.h"

void app_ipc_rx_set_user_handler(void *arg);
void app_register_default_task_handler(void *arg);

void app_entry(void);

extern uint32_t _bss_start, _bss_end;
extern struct sbc_info_t sbc_info;

__attribute__((section("entry_point_section"))) const uint32_t entry_point[] = {
    (uint32_t)app_entry,
};

static void ipc_rx_user_handler(struct ipc_msg_t *msg, uint8_t chn)
{
    uint8_t channel;

    switch(msg->format) {
        case IPC_MSG_DECODED_START:
            {
                struct decoder_param_t {
                    uint32_t example;
                } *decoder_param;
                decoder_param = (struct decoder_param_t *)ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn);
                printf("IPC_MSG_DECODED_START: param is %d", decoder_param->example);

                struct task_msg_t *msg = task_msg_alloc(A2DP_DECODER_INIT, 0);
                task_msg_insert(msg);
            }
            break;
        case IPC_MSG_RAW_FRAME:
            a2dp_decoder_recv_frame(ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn), msg->length);
            break;
        case IPC_MSG_WITHOUT_PAYLOAD:
            switch(msg->length) {
                case IPC_SUB_MSG_DECODER_STOP:
                    {
                        struct task_msg_t *msg = task_msg_alloc(A2DP_DECODER_STOP, 0);
                        task_msg_insert(msg);
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void user_task_handler(struct task_msg_t *msg)
{
    switch(msg->id) {
        case A2DP_DECODER_INIT:
            a2dp_decoder_init(msg);
            break;
        case A2DP_DECODER_STOP:
            a2dp_decoder_release(msg);
            break;
        default:
            break;
    }
}

void app_entry(void)
{
    uint32_t *ptr;
    uint8_t channel;

    for(ptr = &_bss_start; ptr < & _bss_end;) {
        *ptr++ = 0;
    }
    printf("enter app entry: BUILD DATE: %s, TIME: %s\r\n", __DATE__, __TIME__);

    app_ipc_rx_set_user_handler(ipc_rx_user_handler);
    app_register_default_task_handler(user_task_handler);

    /* inform MCU that DSP is ready */
    channel = ipc_alloc_channel(0);
    if(channel != 0xff) {
        ipc_insert_msg(channel, IPC_MSG_WITHOUT_PAYLOAD, IPC_SUM_MSG_DSP_USER_CODE_READY, ipc_free_channel);
    }
}
