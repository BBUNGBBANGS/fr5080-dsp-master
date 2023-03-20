/*
 * native_playback.c
 *
 *  Created on: 2021-7-14
 *      Author: Administrator
 */

#include <stdint.h>
#include <string.h>

#include <xtensa/xtruntime.h>

#include "co_list.h"
#include "co_mem.h"

#include "plf.h"
#include "ipc.h"
#include "tasks.h"

#define NATIVE_PLAYBACK_SINGLE_FRAME        512
#define NATIVE_PLAYBACK_DECODER_THD         (44100*2*2/25)  // 40ms data

struct pcm_data_t {
    struct co_list_hdr hdr;
    uint8_t *data;
    uint32_t offset;
    uint32_t length;
};

static struct co_list pcm_data_list = {NULL, NULL};
static uint32_t pcm_size_in_list = 0;
static uint8_t native_playback_started = 0;

static void native_playback_clear(void)
{
    while(1) {
        struct pcm_data_t *pcm_data = (struct pcm_data_t *)co_list_pop_front(&pcm_data_list);
        if(pcm_data == NULL) {
            break;
        }
        vPortFree(pcm_data->data);
        vPortFree(pcm_data);
    }

    pcm_size_in_list = 0;
}

uint8_t request_in_queue = 0;

__attribute__((section("iram_section"))) static void ipc_tx_memcpy(uint8_t *dst, uint8_t *src, uint32_t size)
{
    uint32_t *dst_u32, *src_u32;

    dst_u32 = (void *)dst;
    src_u32 = (void *)src;
    size >>= 2;
    while(size--) {
        *dst_u32++ = *src_u32++;
    }
}

__attribute__((section("iram_section"))) static void ipc_tx_memset(uint8_t *dst, uint8_t value, uint32_t size)
{
    uint32_t *dst_u32;
    uint32_t value_u32;

    dst_u32 = (void *)dst;
    value_u32 = (value<<24)|(value<<16)|(value<<8)|(value<<0);
    size >>= 2;
    while(size--) {
        *dst_u32++ = value_u32;
    }
}

__attribute__((section("iram_section"))) static void native_playback_ipc_tx(void)
{
    struct pcm_data_t *pcm_data = (struct pcm_data_t *)co_list_pick(&pcm_data_list);
    uint32_t needed_length = NATIVE_PLAYBACK_SINGLE_FRAME;
    uint8_t *spk;

    ipc_dma_tx_int_clear();
//    uart_putc_noint('T');

    if(ipc_get_codec_dac_tog() == 0) {
        spk = (uint8_t *)IPC_DSP_WRITE_BUFFER1;
    }
    else {
        spk = (uint8_t *)IPC_DSP_WRITE_BUFFER0;
    }

    while(pcm_data && (needed_length != 0)) {
        uint32_t last_length;

        last_length = pcm_data->length - pcm_data->offset;
        if(last_length > needed_length) {
//            memcpy(spk, &pcm_data->data[pcm_data->offset], needed_length);
            ipc_tx_memcpy(spk, &pcm_data->data[pcm_data->offset], needed_length);

            pcm_data->offset += needed_length;
            pcm_size_in_list -= needed_length;
            needed_length = 0;
            break;
        }
        else {
//            memcpy(spk, &pcm_data->data[pcm_data->offset], last_length);
            ipc_tx_memcpy(spk, &pcm_data->data[pcm_data->offset], last_length);

            needed_length -= last_length;
            spk += last_length;
            pcm_size_in_list -= last_length;

            pcm_data = (struct pcm_data_t *)co_list_pop_front(&pcm_data_list);
            vPortFree(pcm_data->data);
            vPortFree(pcm_data);
        }
//        pcm_data = (struct pcm_data_t *)co_list_pick(&pcm_data_list);
        pcm_data = (struct pcm_data_t *)pcm_data_list.first;
    }

    if(needed_length) {
//        memset(spk, 0, needed_length);
        ipc_tx_memset(spk, 0, needed_length);
    }

    if(pcm_size_in_list < NATIVE_PLAYBACK_DECODER_THD) {
//        if(request_in_queue < 10)
        {
            struct task_msg_t *msg;
            msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
            task_msg_insert(msg);
            request_in_queue++;
        }
    }
}

__attribute__((section("iram_section"))) void native_playback_ipc_check_irq(void)
{
    if(native_playback_started == 0) {
        return;
    }
    if(*(volatile uint32_t *)0x50000030 & (1<<3)) {
        native_playback_ipc_tx();
    }
}

char *hex2str = "0123456789abcdef";
void native_playback_recv_frame(uint8_t *data, uint32_t length)
{
    struct pcm_data_t *pcm_data;

//    if(request_in_queue > 0) {
//        request_in_queue--;
//    }
//
//    for(uint32_t i=0; i<length; i++) {
//        uart_putc_noint(hex2str[data[i]>>4]);
//        uart_putc_noint(hex2str[data[i]&0x0f]);
//    }
//    printf("\r\n");

    pcm_data = (struct pcm_data_t *)pvPortMalloc(sizeof(struct pcm_data_t));
    pcm_data->data = pvPortMalloc(length);
    memcpy(pcm_data->data, data, length);
    pcm_data->offset = 0;
    pcm_data->length = length;

    GLOBAL_INT_DISABLE();
    co_list_push_back(&pcm_data_list, &pcm_data->hdr);
    pcm_size_in_list += length;
    GLOBAL_INT_RESTORE();
}

void native_playback_init(void)
{
    co_list_init(&pcm_data_list);
    ipc_dma_init(NULL, native_playback_ipc_tx);

    _xtos_interrupt_enable(XCHAL_IPC_DMA_TX_INTERRUPT);

    native_playback_started = 1;
}

void native_playback_destroy(void)
{
    native_playback_started = 0;

    _xtos_interrupt_disable(XCHAL_IPC_DMA_TX_INTERRUPT);

    native_playback_clear();
}