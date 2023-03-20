/*
 * main.c
 *
 *  Created on: 2018-8-19
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>

#include "xa_type_def.h"

#include "ipc.h"
#include "user_def.h"
#include "plf.h"
#include "flash.h"

void app_ipc_rx_set_user_handler(void *arg);
void app_register_default_task_handler(void *arg);

void app_entry(void);

void sbc_decoder_init(void);
void sbc_decoder_recv_frame(uint8_t *buffer, int length);
void sbc_encoder_recv_frame_req(void);

void mp3_decoder_init(void);
void mp3_decoder_recv_frame(pWORD8 buffer, UWORD32 length);
void mp3_decoder_do_decoder_handler(void);

extern uint32_t _bss_start, _bss_end;
extern struct sbc_info_t sbc_info;

__attribute__((section("entry_point_section"))) const uint32_t entry_point[] = {
    (uint32_t)app_entry,
};

static void ipc_rx_user_handler(struct ipc_msg_t *msg, uint8_t chn)
{
    uint8_t channel;

    switch(msg->format) {
        case IPC_MSG_RAW_FRAME:
            mp3_decoder_recv_frame(ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn), msg->length);
            break;
        case IPC_MSG_SET_SBC_CODEC_PARAM:
            memcpy(&sbc_info,ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn),sizeof(struct sbc_info_t));
            break;
        case IPC_MSG_WITHOUT_PAYLOAD:
            switch(msg->length) {
                case IPC_SUB_MSG_DECODER_START:
                    {
                        struct task_msg_t *msg = task_msg_alloc(MP3_DECODER_INIT, 0);
                        task_msg_insert(msg);
                    }
                    break;
                case IPC_SUB_MSG_NEED_MORE_SBC:
                    {
                        struct task_msg_t *msg = task_msg_alloc(MCU_NEED_MORE_SBC_DATA, 0);
                        task_msg_insert(msg);
                    }
                    break;
                case IPC_SUB_MSG_REINIT_DECODER:
                    break;
                case IPC_SUB_MSG_DECODER_PREP_NEXT:
                    // mp3_decoder_prepare_for_next();
                    // sbc_encoder_prepare_for_next();
                    {
                        struct task_msg_t *msg;
                        msg = task_msg_alloc(DECODER_PREPARE_FOR_NEXT, 0);
                        task_msg_insert(msg);
                    }
                    break;
            }
            break;
        default:
            break;
    }
}

void decoder_prepare_for_next_handler(void)
{
    uint8_t channel;

    mp3_decoder_destroy();
    sbc_encoder_destroy();
    channel = ipc_alloc_channel(0);
    if(channel != 0xff) {
        ipc_insert_msg(channel, IPC_MSG_WITHOUT_PAYLOAD, IPC_SUB_MSG_DECODER_PREP_READY, ipc_free_channel);
    }
}

static void user_task_handler(struct task_msg_t *msg)
{
    switch(msg->id) {
        case MP3_DECODER_INIT:
            mp3_decoder_init();
            break;
        case MP3_DECODER_DO_DECODE:
            mp3_decoder_do_decoder_handler();
            break;
        case MCU_NEED_MORE_SBC_DATA:
            sbc_encoder_recv_frame_req();
            break;
        case DECODER_PREPARE_FOR_NEXT:
            decoder_prepare_for_next_handler();
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
