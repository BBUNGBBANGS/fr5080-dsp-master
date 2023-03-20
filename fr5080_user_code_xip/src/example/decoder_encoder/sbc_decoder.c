/*
 * sbc_decoder.c
 *
 *  Created on: 2018-9-3
 *      Author: Administrator
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"
#include "xa_apicmd_standards.h"
#include "xa_memory_standards.h"
#include "sbc_dec/xa_sbc_dec_api.h"

#include "tasks.h"
#include "ipc.h"
#include "co_mem.h"
#include "plf.h"

#define SBC_DECODER_LOG

#define SBC_DECODER_BUFFER_SIZE         2048    // encoded data buffer size
#define SBC_DECODER_BUFFERING_LIMIT     0       // start decoder after receive SBC_DECODER_BUFFERING_LIMIT bytes encoded data

#define MAX_MEM_ALLOCS                  100

#define sbc_decoder_malloc(size)        pvPortMalloc(size)
#define sbc_decoder_free(ptr)           vPortFree(ptr)

enum sbc_decoder_state_t {
    SBC_DECODER_IDLE,
    SBC_DECODER_INITED,
    SBC_DECODER_DECODING,
};

static enum sbc_decoder_state_t sbc_decoder_state = SBC_DECODER_IDLE;
static uint8_t *sbc_decoder_buffer = NULL;
static int sbc_decoder_buffer_input_index = 0;
static int sbc_decoder_buffer_output_index = 0;
static int sbc_decoder_total_input = 0;

static pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
static WORD  g_w_malloc_count = 0;

static pWORD8 pb_inp_buf = 0;
static pWORD8 pb_out_buf = 0;
static UWORD32 ui_inp_size = 0;
static UWORD32 i_buff_size = 0;
static WORD32 i_bytes_consumed = 0;

/* The process API function */
static xa_codec_func_t *p_xa_process_api;
/* API obj */
static xa_codec_handle_t xa_process_handle;

static void dump_data(uint32_t *address, int length)
{
    uint32_t *ptr = address;
    int count = length;
    int i;
    uint32_t *start = (uint32_t *)((uint32_t)ptr & (~0x0f));
    printf("ptr is 0x%08x, count = %d.\r\n", ptr, count);
    for(i=0; i<count;) {
        if(((uint32_t)start & 0x0c) == 0) {
            printf("0x%08x: ", start);
        }
        if(start < ptr) {
            printf("        ");
        }
        else {
            i++;
            printf("%08x", *start);
        }
        if(((uint32_t)start & 0x0c) == 0x0c) {
            printf("\r\n");
        }
        else {
            printf(" ");
        }
        start++;
    }
}

#if 1
static WORD32 xa_sbc_dec_get_data(pVOID buffer, WORD32 size)
{
    int length, deal_length;

    SBC_DECODER_LOG("xa_sbc_dec_get_data: buffer=0x%08x, size=%d\r\n", buffer, size);
    SBC_DECODER_LOG("xa_sbc_dec_get_data: (BEFORE) sbc_decoder_buffer_output_index=%d\r\n", sbc_decoder_buffer_output_index);

    GLOBAL_INT_DISABLE();
    if(sbc_decoder_buffer_output_index < sbc_decoder_buffer_input_index) {
        length = sbc_decoder_buffer_input_index - sbc_decoder_buffer_output_index;
        if(length < size) {
            size = length;
        }
        memcpy(buffer, &sbc_decoder_buffer[sbc_decoder_buffer_output_index], size);
        sbc_decoder_buffer_output_index += size;

        deal_length = size;
    }
    else if(sbc_decoder_buffer_output_index == sbc_decoder_buffer_input_index) {
        deal_length = 0;
    }
    else {
        length = SBC_DECODER_BUFFER_SIZE - sbc_decoder_buffer_output_index;
        if(length >= size) {
            length = size;
        }
        memcpy(buffer, &sbc_decoder_buffer[sbc_decoder_buffer_output_index], length);
        sbc_decoder_buffer_output_index += length;
        if(sbc_decoder_buffer_output_index == SBC_DECODER_BUFFER_SIZE) {
            sbc_decoder_buffer_output_index = 0;
        }
        deal_length = length;
        if(length != size) {
            size -= length;
            deal_length += xa_sbc_dec_get_data((uint8_t *)buffer+deal_length, size);
        }
    }
    GLOBAL_INT_RESTORE();

    SBC_DECODER_LOG("xa_sbc_dec_get_data: (BEFORE) sbc_decoder_buffer_output_index=%d\r\n", sbc_decoder_buffer_output_index);

    return deal_length;
}

static XA_ERRORCODE xa_sbc_dec_get_config_param (xa_codec_handle_t p_xa_process_api_obj,
                 pWORD32           pi_bitrate,
                 pWORD32           pi_samp_freq,
                 pWORD32           pi_num_chan,
                 pWORD32           pi_pcm_wd_sz)
{
  XA_ERRORCODE err_code = XA_NO_ERROR;
  /* the process API function */
  xa_codec_func_t *p_xa_process_api = xa_sbc_dec;

  /* Data rate */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                   XA_API_CMD_GET_CONFIG_PARAM,
                   XA_SBC_DEC_CONFIG_PARAM_BITRATE, pi_bitrate);

  }
  /* Sampling frequency */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                   XA_API_CMD_GET_CONFIG_PARAM,
                   XA_SBC_DEC_CONFIG_PARAM_SAMP_FREQ, pi_samp_freq);

  }
  /* Number of channels */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                   XA_API_CMD_GET_CONFIG_PARAM,
                   XA_SBC_DEC_CONFIG_PARAM_NUM_CHANNELS, pi_num_chan);
  }
  /* PCM word size */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                   XA_API_CMD_GET_CONFIG_PARAM,
                   XA_SBC_DEC_CONFIG_PARAM_PCM_WDSZ, pi_pcm_wd_sz);
  }
  return XA_NO_ERROR;
}

static void sbc_decoder_engine_init(void)
{
    UWORD32 pui_api_size = 0;
    XA_ERRORCODE err_code = XA_NO_ERROR;

    UWORD32 i;
    UWORD32 n_mems = 0;
    UWORD32 ui_proc_mem_tabs_size = 0;

    p_xa_process_api = xa_sbc_dec;

    /* ******************************************************************/
    /* Initialize API structure and set config params to default        */
    /* ******************************************************************/

    /* Get the API size */
    err_code = (*p_xa_process_api)(0, XA_API_CMD_GET_API_SIZE, 0,
                 &pui_api_size);

    g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_decoder_malloc(pui_api_size);
    /* Set API object with the memory allocated */
    xa_process_handle = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

    g_w_malloc_count++;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(xa_process_handle,
                 XA_API_CMD_INIT,
                 XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                 0);

    /* ******************************************************************/
    /* Initialize Memory info tables                                    */
    /* ******************************************************************/

    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(xa_process_handle,
                 XA_API_CMD_GET_MEMTABS_SIZE, 0,
                 &ui_proc_mem_tabs_size);

    /* Memory table requires 4 bytes (WORD32) alignment; malloc()
     * provides at least 8-byte alignment.
     */
    g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_decoder_malloc(ui_proc_mem_tabs_size+3);

    /* Set pointer for process memory tables    */
    err_code = (*p_xa_process_api)(xa_process_handle,
                 XA_API_CMD_SET_MEMTABS_PTR, 0,
                 (void *) (((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] + 3) & (~0x03)));

    g_w_malloc_count++;

    /* initialize the API, post config, fill memory tables  */
    err_code = (*p_xa_process_api)(xa_process_handle,
                 XA_API_CMD_INIT,
                 XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
                 0);

    /* ******************************************************************/
    /* Allocate Memory with info from library                           */
    /* ******************************************************************/

    /* Get number of memory tables required */
    err_code = (*p_xa_process_api)(xa_process_handle,
                 XA_API_CMD_GET_N_MEMTABS,
                 0,
                 &n_mems);
    SBC_DECODER_LOG("sbc_decoder_engine_init: n_mems=%d.\r\n", n_mems);

    for (i = 0; i < (WORD32) n_mems; i++) {
        int ui_size, ui_alignment, ui_type;
        pVOID pv_alloc_ptr;

        /* Get memory size */
        err_code = (*p_xa_process_api)(xa_process_handle,
                       XA_API_CMD_GET_MEM_INFO_SIZE,
                       i,
                       &ui_size);

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(xa_process_handle,
                       XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
                       i,
                       &ui_alignment);

        /* Get memory type */
        err_code = (*p_xa_process_api)(xa_process_handle,
                       XA_API_CMD_GET_MEM_INFO_TYPE,
                       i,
                       &ui_type);

        g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_decoder_malloc(ui_size + ui_alignment - 1);

        pv_alloc_ptr = (void *) (((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] + ui_alignment - 1) & (~(ui_alignment - 1)));

        g_w_malloc_count++;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(xa_process_handle,
                       XA_API_CMD_SET_MEM_PTR,
                       i,
                       pv_alloc_ptr);

        if(ui_type == XA_MEMTYPE_INPUT) {
            pb_inp_buf = pv_alloc_ptr;
            ui_inp_size = ui_size;
            i_buff_size = ui_size;
            i_bytes_consumed = ui_size;
            SBC_DECODER_LOG("sbc_decoder_engine_init: ui_inp_size=%d.\r\n", ui_inp_size);
        }
        if(ui_type == XA_MEMTYPE_OUTPUT) {
            pb_out_buf = pv_alloc_ptr;
        }
    }
    /* End first part */
}

static UWORD32 sbc_decoder_engine_start(void)
{
    WORD32 i_bytes_read = 0;
    UWORD32 i;
    XA_ERRORCODE err_code = XA_NO_ERROR;
    UWORD32 ui_init_done = 0;
    UWORD32 last_data_length;

    last_data_length = i_buff_size - i_bytes_consumed;
    memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], last_data_length);
    i_bytes_read = xa_sbc_dec_get_data(pb_inp_buf + last_data_length,
                                            ui_inp_size-last_data_length);
    i_buff_size = last_data_length + i_bytes_read;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_INPUT_BYTES,
                   0,
                   &i_buff_size);
    SBC_DECODER_LOG("sbc_decoder_engine_start: XA_API_CMD_SET_INPUT_BYTES--err=%d.\r\n", err_code);

    /* Initialize the process */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_PROCESS,
                   0);
    SBC_DECODER_LOG("sbc_decoder_engine_start: XA_CMD_TYPE_INIT_PROCESS--err=%d.\r\n", err_code);

    /* Checking for end of initialization */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_DONE_QUERY,
                   &ui_init_done);
    SBC_DECODER_LOG("sbc_decoder_engine_start: XA_CMD_TYPE_INIT_DONE_QUERY--err=%d.\r\n", err_code);

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                   0,
                   &i_bytes_consumed);

    SBC_DECODER_LOG("sbc_decoder_engine_start: i_bytes_consumed is %d, ui_init_done is %d\r\n", i_bytes_consumed, ui_init_done);

    return ui_init_done;
}

static void sbc_decoder_engine_decode(void)
{
    WORD32 i_bytes_read = 0;
    UWORD32 i;
    XA_ERRORCODE err_code = XA_NO_ERROR;
    UWORD32 ui_exec_done = 0;
    UWORD32 i_out_bytes;
    UWORD32 last_data_length;

    last_data_length = i_buff_size - i_bytes_consumed;
    if((last_data_length != 0) && (i_bytes_consumed != 0)) {
        memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], last_data_length);
    }
    i_bytes_read = xa_sbc_dec_get_data(pb_inp_buf + last_data_length,
                                            ui_inp_size - last_data_length);
    if((i_bytes_consumed !=0) && (i_bytes_read == 0)) {
        return;
    }

    /* New buffer size */
    i_buff_size = i_bytes_read + last_data_length;
    SBC_DECODER_LOG("sbc_decoder_engine_decode: i_buff_size=%d, i_bytes_read=%d.\r\n", i_buff_size, i_bytes_read);

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_INPUT_BYTES,
                   0,
                   &i_buff_size);
    SBC_DECODER_LOG("sbc_decoder_engine_decode: XA_API_CMD_SET_INPUT_BYTES--err=%d.\r\n", err_code);

    /* Execute process */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_EXECUTE,
                   XA_CMD_TYPE_DO_EXECUTE,
                   0);
    SBC_DECODER_LOG("sbc_decoder_engine_decode: XA_CMD_TYPE_DO_EXECUTE--err=%d.\r\n", err_code);

    /* Checking for end of processing */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_EXECUTE,
                   XA_CMD_TYPE_DONE_QUERY,
                   &ui_exec_done);
    SBC_DECODER_LOG("sbc_decoder_engine_decode: XA_CMD_TYPE_DONE_QUERY--err=%d, ui_exec_done=%d.\r\n", err_code, ui_exec_done);

    /* Get the output bytes */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_GET_OUTPUT_BYTES,
                   0,
                   &i_out_bytes);
    SBC_DECODER_LOG("sbc_decoder_engine_decode: XA_API_CMD_GET_OUTPUT_BYTES--err=%d, i_out_bytes=%d.\r\n", err_code, i_out_bytes);
    if(i_out_bytes) {
//        uint8_t channel;
//        uint8_t *buffer;
//        channel = ipc_alloc_channel(i_out_bytes);
//        if(channel != 0xff) {
//            buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
//            memcpy(buffer, pb_out_buf, i_out_bytes);
//            ipc_insert_msg(channel, 4/*IPC_MSG_DECODED_FRAME*/, i_out_bytes, ipc_free_channel);
//        }
    }

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                   0,
                   &i_bytes_consumed);
    SBC_DECODER_LOG("sbc_decoder_engine_decode: XA_API_CMD_GET_CURIDX_INPUT_BUF--err=%d, i_bytes_consumed=%d.\r\n", err_code, i_bytes_consumed);

    if(i_bytes_consumed != 0) {
        struct task_msg_t *msg;
        msg = task_msg_alloc(SBC_DECODER_DO_DECODE, 0);
        task_msg_insert(msg);
    }
}
#endif

void sbc_decoder_init(void)
{
    sbc_decoder_buffer = sbc_decoder_malloc(SBC_DECODER_BUFFER_SIZE);
}

void sbc_decoder_recv_frame(uint8_t *buffer, int length)
{
    struct task_msg_t *msg;
    int last_space = SBC_DECODER_BUFFER_SIZE - sbc_decoder_buffer_input_index;

    sbc_decoder_total_input += length;

    if(sbc_decoder_state == SBC_DECODER_IDLE) {
        sbc_decoder_buffer = sbc_decoder_malloc(SBC_DECODER_BUFFER_SIZE);
        sbc_decoder_engine_init();
        sbc_decoder_state = SBC_DECODER_INITED;
    }

    SBC_DECODER_LOG("sbc_decoder_recv_frame: (BEFORE) input_length=%d, sbc_decoder_buffer_input_index=%d.\r\n", length, sbc_decoder_buffer_input_index);

    if(last_space <= length) {
        memcpy(sbc_decoder_buffer+sbc_decoder_buffer_input_index, buffer, last_space);
        length -= last_space;
        buffer += last_space;
        sbc_decoder_buffer_input_index = 0;
        if(length) {
            memcpy(sbc_decoder_buffer, buffer, length);
            sbc_decoder_buffer_input_index = length;
        }
    }
    else {
        memcpy(sbc_decoder_buffer+sbc_decoder_buffer_input_index, buffer, length);
        sbc_decoder_buffer_input_index += length;
        if(sbc_decoder_buffer_input_index >= SBC_DECODER_BUFFER_SIZE) {
            sbc_decoder_buffer_input_index = 0;
        }
    }
    SBC_DECODER_LOG("sbc_decoder_recv_frame: (AFTER) input_length=%d, sbc_decoder_buffer_input_index=%d.\r\n", length, sbc_decoder_buffer_input_index);

    msg = task_msg_alloc(SBC_DECODER_DO_DECODE, 0);
    task_msg_insert(msg);
}

void sbc_decoder_do_decoder_handler(void *arg)
{
    if(sbc_decoder_state == SBC_DECODER_INITED) {
        if(sbc_decoder_engine_start()) {
            sbc_decoder_state = SBC_DECODER_DECODING;
            sbc_decoder_engine_decode();
        }
    }
    else {
        sbc_decoder_engine_decode();
    }
}
