/*
 * main.c
 *
 *  Created on: 2018-8-19
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>
#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "ipc.h"
#include "plf.h"
#include "tasks.h"
#include "user_def.h"
#include "audio_algorithm.h"

void app_entry(void);
void sbc_decoder_recv_frame(uint8_t *buffer, int length);

void app_ipc_rx_set_user_handler(void *arg);

void audio_init(void *msg);
void audio_algorithm_release(void *msg);
extern void audio_algorithm_ipc_rx_handler(struct task_msg_t *msg);
extern void audio_algorithm_ipc_tx_handler(struct task_msg_t *msg);

extern uint32_t _bss_start, _bss_end;

__attribute__((section("entry_point_section"))) const uint32_t entry_point[] = {
    (uint32_t)app_entry,
};

void ipc_rx_user_handler(struct ipc_msg_t *msg, uint8_t chn)
{
    uint8_t channel;

    printf("ipc_rx_user_handler: recv format: %d.\r\n", msg->format);

    switch(msg->format) {
        case IPC_MSG_RAW_FRAME:
            sbc_decoder_recv_frame(ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn), msg->length);
            break;
		case IPC_MSG_WITHOUT_PAYLOAD:
			switch(msg->length){
				case IPC_SUB_MSG_NREC_START:
				    printf("audio algorithm start\r\n");
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(AUDIO_ALGO_CREATE, 0);
                        task_msg_insert(msg);
                    }
                    break;
				case IPC_SUB_MSG_NREC_STOP:
				    printf("audio algorithm release\r\n");
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(AUDIO_ALGO_DESTROY, 0);
                        task_msg_insert(msg);
                    }
                    break;
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
        case AUDIO_IPC_DMA_RX:
            audio_algorithm_ipc_rx_handler(msg);
            break;
        case AUDIO_IPC_DMA_TX:
            audio_algorithm_ipc_tx_handler(msg);
            break;
        case AUDIO_ALGO_CREATE:
            audio_init(msg);
            break;
        case AUDIO_ALGO_DESTROY:
            audio_algorithm_release(msg);
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
