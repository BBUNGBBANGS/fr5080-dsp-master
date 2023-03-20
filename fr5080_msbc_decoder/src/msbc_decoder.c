/*
 * msbc_decoder.c
 *
 *  Created on: 2021-11-4
 *      Author: Administrator
 */


#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#if __XCC__
#include <xtensa/hal.h>
#include <xtensa/tie/xt_hifi2.h>
#include <xtensa/sim.h>
#endif
#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"
#include "xa_apicmd_standards.h"
#include "xa_memory_standards.h"
#include "msbc_dec/xa_msbc_dec_api.h"

#include "tasks.h"
#include "ipc.h"
#include "co_mem.h"
#include "plf.h"
#include "user_def.h"
#include "upsample.h"
#include "downsample.h"

#define USE_DOWNSAMPLE                  1

#define MSBC_LOG                        //printf
#define MSBC_LOG_ERR                    //printf

#define MAX_MEM_ALLOCS                  100

#define MSBC_SINGLE_FRAME_SIZE          57

/* send IPC_MSG_RAW_BUFFER_SPACE to request more data after MSBC_DECODER_REQUEST_DATA_THD bytes have been consumed */
#define MSBC_DECODER_REQUEST_DATA_THD   (MSBC_SINGLE_FRAME_SIZE*8)

/* MP3 decoder input buffer size */
#define MSBC_DECODER_BUFFER_SIZE         (MSBC_SINGLE_FRAME_SIZE*64)
/* Start decoder after how many bytes have been received */
#define MSBC_DECODER_BUFFERING_LIMIT     (MSBC_SINGLE_FRAME_SIZE*16)
/* indicate the limitation of apply size */
#define MSBC_DECODER_APPLY_LIMIT         (MSBC_SINGLE_FRAME_SIZE*32)

enum msbc_decoder_state_t {
    MSBC_DECODER_IDLE,
    MSBC_DECODER_BUFFERING,
    MSBC_DECODER_INITIATING,
    MSBC_DECODER_DECODING,
    MSBC_DECODER_PCM_TRANSMITTING,
};

/* The process API function */
static xa_codec_func_t *p_xa_process_api;
/* API obj */
static xa_codec_handle_t xa_process_handle;

/* Process initing done query variable */
static UWORD32 ui_init_done, ui_exec_done;
static pWORD8 pb_inp_buf, pb_out_buf;
static UWORD32 ui_inp_size;
static WORD32 i_bytes_consumed, i_bytes_read;
static WORD32 i_buff_size;

/* MP3 decoder input buffer */
static pWORD8 msbc_decoder_buffer = NULL;
static UWORD32 msbc_decoder_buffer_input_index = 0;
static UWORD32 msbc_decoder_buffer_output_index = 0;
static UWORD32 msbc_decoder_total_input = 0;

static pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
static WORD  g_w_malloc_count = 0;

/* how many bytes have been consumed since IPC_MSG_RAW_BUFFER_SPACE is sent */
static UWORD32 msbc_decoder_dynamic_consumed = 0;
static bool msbc_decoder_all_request_data_received = false;
static UWORD32 msbc_decoder_request_data_size = 0;
static bool msbc_decoder_prep_for_next = false;
static bool msbc_ignore_first_consumed = false;

static enum msbc_decoder_state_t msbc_decoder_state = MSBC_DECODER_IDLE;

static bool resample_used = false;

void native_playback_init(void);
void native_playback_destroy(void);
void native_playback_recv_frame(uint8_t *data, uint32_t length);

static void *msbc_mem_alloc(UWORD32 length)
{
    void *ptr = pvPortMalloc_user(length);
    if(ptr == NULL) {
        MSBC_LOG_ERR("msbc_mem_alloc: failed.\r\n");
    }

    MSBC_LOG("msbc_mem_alloc: ptr=0x%08x, length=%d.\r\n", ptr, length);

    return ptr;
}

static void msbc_mem_free(void *buffer)
{
    vPortFree_user((void *)buffer);
}

static WORD32 xa_sbc_dec_get_data(pVOID buffer, WORD32 size)
{
    int length, deal_length;

    GLOBAL_INT_DISABLE();
    MSBC_LOG("xa_sbc_dec_get_data start: msbc_decoder_buffer_output_index=%d.\r\n", msbc_decoder_buffer_output_index);
    if(msbc_decoder_buffer_output_index < msbc_decoder_buffer_input_index) {
        length = msbc_decoder_buffer_input_index - msbc_decoder_buffer_output_index;
        if(length < size) {
            size = length;
        }
        memcpy(buffer, &msbc_decoder_buffer[msbc_decoder_buffer_output_index], size);
        msbc_decoder_buffer_output_index += size;

        deal_length = size;
    }
    else if(msbc_decoder_buffer_output_index == msbc_decoder_buffer_input_index) {
        deal_length = 0;
    }
    else {
        length = MSBC_DECODER_BUFFER_SIZE - msbc_decoder_buffer_output_index;
        if(length >= size) {
            length = size;
        }
        memcpy(buffer, &msbc_decoder_buffer[msbc_decoder_buffer_output_index], length);
        msbc_decoder_buffer_output_index += length;
        if(msbc_decoder_buffer_output_index == MSBC_DECODER_BUFFER_SIZE) {
            msbc_decoder_buffer_output_index = 0;
        }
        deal_length = length;
        if(length != size) {
            size -= length;
            deal_length += xa_sbc_dec_get_data((uint8_t *)buffer+deal_length, size);
        }
    }
    MSBC_LOG("xa_sbc_dec_get_data end: msbc_decoder_buffer_output_index=%d.\r\n", msbc_decoder_buffer_output_index);
    GLOBAL_INT_RESTORE();

    return deal_length;
}

static void msbc_decoder_engine_decode(void)
{
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    XA_ERRORCODE err_code_exec = XA_NO_ERROR;
    UWORD32 left_data_length, i_out_bytes;
    xa_msbc_frame_type_t frame_type;

    left_data_length = i_buff_size - i_bytes_consumed;
    if((left_data_length != 0) && (i_bytes_consumed != 0)) {
        memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], left_data_length);
    }
    i_bytes_read = xa_sbc_dec_get_data(pb_inp_buf + left_data_length,
                                            ui_inp_size - left_data_length);
    i_buff_size = left_data_length + i_bytes_read;

    if((msbc_ignore_first_consumed == false) && (i_bytes_consumed == 0) && (i_bytes_read == 0)) {
        return;
    }
    msbc_ignore_first_consumed = false;
    i_bytes_consumed = 0;

//    uart_putc_noint('<');

    /* Set number of bytes to be processed */
    MSBC_LOG_ERR("input size %d.\r\n", i_buff_size);
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_INPUT_BYTES,
                   0,
                   &i_buff_size);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_decode: XA_API_CMD_SET_INPUT_BYTES %08x.\r\n", err_code);
    }

    /* Send the frame type information to the API */
    frame_type = XA_MSBC_GOOD_FRAME;
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_CONFIG_PARAM,
                   XA_MSBC_DEC_CONFIG_PARAM_FRAME_TYPE,
                   &frame_type);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_decode: XA_MSBC_DEC_CONFIG_PARAM_FRAME_TYPE %08x.\r\n", err_code);
    }

    /* Execute process */
    err_code_exec = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_EXECUTE,
                   XA_CMD_TYPE_DO_EXECUTE,
                   NULL);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_decode: XA_CMD_TYPE_DO_EXECUTE %08x.\r\n", err_code);
    }

    /* Checking for end of processing */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_EXECUTE,
                   XA_CMD_TYPE_DONE_QUERY,
                   &ui_exec_done);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_decode: XA_CMD_TYPE_DONE_QUERY %08x.\r\n", err_code);
    }

    /* Get the output bytes */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_GET_OUTPUT_BYTES,
                   0,
                   &i_out_bytes);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_decode: XA_API_CMD_GET_OUTPUT_BYTES %08x.\r\n", err_code);
    }

    if(i_out_bytes) {
        if(resample_used) {
            uart_putc_noint('[');
#if USE_DOWNSAMPLE == 1
            downsample_entity(pb_out_buf, i_out_bytes, native_playback_recv_frame);
#else
            upsample_entity(pb_out_buf, i_out_bytes, native_playback_recv_frame);
#endif
            uart_putc_noint(']');
        }
        else {
            native_playback_recv_frame(pb_out_buf, i_out_bytes);
//            char *hex2char = "0123456789abcdef";
//            for(uint32_t i=0; i<i_out_bytes; i++) {
//                uint8_t c = pb_out_buf[i];
//                uart_putc_noint(hex2char[c>>4]);
//                uart_putc_noint(hex2char[c&0x0f]);
//            }
//            uart_putc_noint('\r');
//            uart_putc_noint('\n');
        }
    }

error_exit:
    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
               XA_API_CMD_GET_CURIDX_INPUT_BUF,
               0,
               &i_bytes_consumed);
    MSBC_LOG_ERR("consumed=%d, i_out_bytes=%d.\r\n", i_bytes_consumed, i_out_bytes);

    msbc_decoder_dynamic_consumed += i_bytes_consumed;

    MSBC_LOG("msbc_decoder_dynamic_consumed is %d.\r\n", msbc_decoder_dynamic_consumed);
    /* apply more raw data after more than MSBC_DECODER_REQUEST_DATA_THD bytes have been consumed */
    if((msbc_decoder_all_request_data_received == true)
        && (msbc_decoder_dynamic_consumed >= MSBC_DECODER_REQUEST_DATA_THD)
        && (msbc_decoder_prep_for_next == false)) {
        uint8_t channel = ipc_alloc_channel(0);
        UWORD32 left_space;

        if(channel != 0xff) {
            MSBC_LOG("apply more data, %d, %d.\r\n", msbc_decoder_buffer_output_index, msbc_decoder_buffer_input_index);
            GLOBAL_INT_DISABLE();
            if(msbc_decoder_buffer_output_index > msbc_decoder_buffer_input_index) {
                left_space = msbc_decoder_buffer_input_index + MSBC_DECODER_BUFFER_SIZE - msbc_decoder_buffer_output_index;
            }
            else {
                left_space = msbc_decoder_buffer_input_index - msbc_decoder_buffer_output_index;
            }
            GLOBAL_INT_RESTORE();
            uint32_t apply_unit;
            if((MSBC_DECODER_BUFFER_SIZE-left_space) > MSBC_DECODER_APPLY_LIMIT) {
                apply_unit = MSBC_DECODER_APPLY_LIMIT/MSBC_SINGLE_FRAME_SIZE - 1;
            }
            else {
                apply_unit = (MSBC_DECODER_BUFFER_SIZE-left_space)/MSBC_SINGLE_FRAME_SIZE - 1;
            }
            if(apply_unit) {
                MSBC_LOG_ERR("apply more data, %d.\r\n", apply_unit);
                ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);
                msbc_decoder_dynamic_consumed = 0;

                msbc_decoder_all_request_data_received = false;
                msbc_decoder_request_data_size = apply_unit*MSBC_SINGLE_FRAME_SIZE;
            }
            else {
                ipc_free_channel(channel);
            }
        }
    }

//    uart_putc_noint('>');
}

static WORD8 msbc_decoder_engine_start(void)
{
    UWORD32 left_data_length;
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    UWORD32 plc_option = 1;

    left_data_length = i_buff_size - i_bytes_consumed;
    memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], left_data_length);
    i_bytes_read = xa_sbc_dec_get_data(pb_inp_buf + left_data_length,
                                            ui_inp_size-left_data_length);
    i_buff_size = left_data_length + i_bytes_read;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_INPUT_BYTES,
                   0,
                   &i_buff_size);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_start: XA_API_CMD_SET_INPUT_BYTES %08x.\r\n", err_code);
    }

    /* Send the PLC information to the API */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_SET_CONFIG_PARAM,
                   XA_MSBC_DEC_CONFIG_PARAM_PLC_OPTION,
                   &plc_option);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_start: XA_API_CMD_SET_CONFIG_PARAM %08x.\r\n", err_code);
    }

    /* Initialize the process */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_PROCESS,
                   NULL);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_start: XA_CMD_TYPE_INIT_PROCESS %08x.\r\n", err_code);
    }

    /* Checking for end of initialization */
    err_code = (*p_xa_process_api)(xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_DONE_QUERY,
                   &ui_init_done);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("msbc_decoder_engine_start: XA_CMD_TYPE_INIT_DONE_QUERY %08x.\r\n", err_code);
    }

    if(ui_init_done) {

    }

error_exit:
    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_CURIDX_INPUT_BUF,
        0,
        &i_bytes_consumed);
    MSBC_LOG("msbc_decoder_engine_start: XA_API_CMD_GET_CURIDX_INPUT_BUF--err%04x, i_bytes_consumed=%d.\r\n", err_code, i_bytes_consumed);

    msbc_decoder_dynamic_consumed += i_bytes_consumed;

    /* apply more raw data after more than MSBC_DECODER_REQUEST_DATA_THD bytes have been consumed */
    if((msbc_decoder_all_request_data_received == true)
        && (msbc_decoder_dynamic_consumed >= MSBC_DECODER_REQUEST_DATA_THD)
        && (msbc_decoder_prep_for_next == false)) {
        uint8_t channel = ipc_alloc_channel(0);
        UWORD32 left_space;

        if(channel != 0xff) {
            MSBC_LOG("apply more data, %d, %d.\r\n", msbc_decoder_buffer_output_index, msbc_decoder_buffer_input_index);
            GLOBAL_INT_DISABLE();
            if(msbc_decoder_buffer_output_index > msbc_decoder_buffer_input_index) {
                left_space = msbc_decoder_buffer_input_index + MSBC_DECODER_BUFFER_SIZE - msbc_decoder_buffer_output_index;
            }
            else {
                left_space = msbc_decoder_buffer_input_index - msbc_decoder_buffer_output_index;
            }
            GLOBAL_INT_RESTORE();
            uint32_t apply_unit;
            if((MSBC_DECODER_BUFFER_SIZE-left_space) > MSBC_DECODER_APPLY_LIMIT) {
                apply_unit = MSBC_DECODER_APPLY_LIMIT/MSBC_SINGLE_FRAME_SIZE - 1;
            }
            else {
                apply_unit = (MSBC_DECODER_BUFFER_SIZE-left_space)/MSBC_SINGLE_FRAME_SIZE - 1;
            }
            if(apply_unit) {
                MSBC_LOG("apply more data, %d.\r\n", apply_unit);
                ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);
                msbc_decoder_dynamic_consumed = 0;

                msbc_decoder_all_request_data_received = false;
                msbc_decoder_request_data_size = apply_unit*MSBC_SINGLE_FRAME_SIZE;
            }
            else {
                ipc_free_channel(channel);
            }
        }
    }

    if(ui_init_done) {
        if(i_bytes_consumed) {
            msbc_ignore_first_consumed = false;
        }
        else {
            msbc_ignore_first_consumed = true;
        }
    }

    return ui_init_done;
}

static void msbc_decoder_engine_init(void)
{
    LOOPIDX i;
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    /* Memory variables */
    UWORD32 n_mems;
    /* API size */
    UWORD32 pui_api_size;
    UWORD32 ui_proc_mem_tabs_size;

    /* Stack process struct initing */
    p_xa_process_api = xa_msbc_dec;

    /* ******************************************************************/
    /* Initialize API structure and set config params to default        */
    /* ******************************************************************/

    /* Get the API size */
    err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0,
                                    &pui_api_size);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_API_CMD_GET_API_SIZE %08x.\r\n", err_code);
    }

    /* Allocate memory for API */
    g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_mem_alloc(pui_api_size);

    /* Set API object with the memory allocated */
    xa_process_handle = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

    g_w_malloc_count++;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_INIT,
                                    XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                                    NULL);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS %08x.\r\n", err_code);
    }

    /* ******************************************************************/
    /* Initialize Memory info tables                                    */
    /* ******************************************************************/

    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_MEMTABS_SIZE, 0,
                                    &ui_proc_mem_tabs_size);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_API_CMD_GET_MEMTABS_SIZE %08x.\r\n", err_code);
    }

    g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_mem_alloc(ui_proc_mem_tabs_size);

    /* Set pointer for process memory tables  */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_SET_MEMTABS_PTR, 0,
                                    (void *) g_pv_arr_alloc_memory[g_w_malloc_count]);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_API_CMD_SET_MEMTABS_PTR %08x.\r\n", err_code);
    }

    g_w_malloc_count++;

    /* initialize the API, post config, fill memory tables    */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_INIT,
                                    XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
                                    NULL);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS %08x.\r\n", err_code);
    }

    /* ******************************************************************/
    /* Allocate Memory with info from library                           */
    /* ******************************************************************/

    /* Get number of memory tables required */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_N_MEMTABS,
                                    0,
                                    &n_mems);
    if(err_code != XA_NO_ERROR) {
        MSBC_LOG_ERR("XA_API_CMD_GET_N_MEMTABS %08x.\r\n", err_code);
    }

    for (i = 0; i < (WORD32) n_mems; i++) {
        int ui_size, ui_alignment, ui_type;
        pVOID pv_alloc_ptr;

        /* Get memory size */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_MEM_INFO_SIZE,
                                    i,
                                    &ui_size);
        if(err_code != XA_NO_ERROR) {
            MSBC_LOG_ERR("XA_API_CMD_GET_MEM_INFO_SIZE %08x.\r\n", err_code);
        }

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
                                    i,
                                    &ui_alignment);
        if(err_code != XA_NO_ERROR) {
            MSBC_LOG_ERR("XA_API_CMD_GET_MEM_INFO_ALIGNMENT %08x.\r\n", err_code);
        }

        /* Get memory type */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_MEM_INFO_TYPE,
                                    i,
                                    &ui_type);
        if(err_code != XA_NO_ERROR) {
            MSBC_LOG_ERR("XA_API_CMD_GET_MEM_INFO_TYPE %08x.\r\n", err_code);
        }

        g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_mem_alloc(ui_size);

        pv_alloc_ptr = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

        g_w_malloc_count++;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_SET_MEM_PTR,
                                    i,
                                    pv_alloc_ptr);
        if(err_code != XA_NO_ERROR) {
            MSBC_LOG_ERR("XA_API_CMD_SET_MEM_PTR %08x.\r\n", err_code);
        }

        if(ui_type == XA_MEMTYPE_INPUT) {
            pb_inp_buf = pv_alloc_ptr;
            ui_inp_size = ui_size;
        }
        if(ui_type == XA_MEMTYPE_OUTPUT) {
            pb_out_buf = pv_alloc_ptr;
        }
    }
}

static const char *msbc_decoder_state_str[] = {
    "MSBC_DECODER_IDLE",
    "MSBC_DECODER_BUFFERING",
    "MSBC_DECODER_INITIATING",
    "MSBC_DECODER_DECODING",
    "MSBC_DECODER_PCM_TRANSMITTING",
};

void msbc_decoder_do_decoder_handler(void *arg)
{
    MSBC_LOG("msbc_decoder_do_decoder_handler: msbc_decoder_state = %s.\r\n", msbc_decoder_state_str[msbc_decoder_state]);
    uint32_t available_data = 0;

    switch(msbc_decoder_state) {
        case MSBC_DECODER_IDLE:
            msbc_decoder_state = MSBC_DECODER_BUFFERING;
        case MSBC_DECODER_BUFFERING:
            if(MSBC_DECODER_BUFFERING_LIMIT <= msbc_decoder_total_input) {
                msbc_decoder_engine_init();
                msbc_decoder_state = MSBC_DECODER_INITIATING;
            }
            else {
                break;
            }
        case MSBC_DECODER_INITIATING:
            if(msbc_decoder_engine_start() == 1) {
                /* once initialization is done, start decode the first frame directly */
                msbc_decoder_state = MSBC_DECODER_DECODING;
            }
            else {
                /* try to restart initialization if start failed */
                if(msbc_decoder_prep_for_next == false) {
                    struct task_msg_t *msg;
                    msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
                    task_msg_insert(msg);
                }
                break;
            }
        case MSBC_DECODER_DECODING:
            msbc_decoder_engine_decode();
            break;
        case MSBC_DECODER_PCM_TRANSMITTING:
        default:
            break;
    }
}

void msbc_decoder_init(void *arg)
{
    uint8_t channel;

    channel = ipc_alloc_channel(0);
    if(channel != 0xff) {
        ui_init_done = 0;
        ui_exec_done = 0;

        msbc_decoder_buffer = msbc_mem_alloc(MSBC_DECODER_BUFFER_SIZE);

        msbc_decoder_buffer_input_index = 0;
        msbc_decoder_buffer_output_index = 0;

        msbc_decoder_dynamic_consumed = 0;
        msbc_decoder_total_input = 0;
        msbc_decoder_state = MSBC_DECODER_IDLE;

        uint32_t apply_unit;
        if(MSBC_DECODER_APPLY_LIMIT < MSBC_DECODER_BUFFER_SIZE) {
            apply_unit = MSBC_DECODER_APPLY_LIMIT/MSBC_SINGLE_FRAME_SIZE - 1;
        }
        else {
            apply_unit = MSBC_DECODER_BUFFER_SIZE/MSBC_SINGLE_FRAME_SIZE - 1;
        }
        ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);

        msbc_decoder_all_request_data_received = false;
        msbc_decoder_request_data_size = apply_unit*MSBC_SINGLE_FRAME_SIZE;

        msbc_decoder_prep_for_next = false;

        resample_used = false;

        native_playback_init();
    }
}

void msbc_decoder_destroy(void)
{
    uint32_t i;

    for(i=0; i<g_w_malloc_count; i++) {
        msbc_mem_free(g_pv_arr_alloc_memory[i]);
    }
    g_w_malloc_count = 0;

    msbc_mem_free(msbc_decoder_buffer);
    msbc_decoder_buffer = NULL;

    msbc_decoder_dynamic_consumed = 0;
    msbc_decoder_total_input = 0;
    msbc_decoder_state = MSBC_DECODER_IDLE;
    msbc_decoder_all_request_data_received = false;

    if(resample_used) {
#if USE_DOWNSAMPLE == 1
        downsample_destroy();
#else   // USE_DOWNSAMPLE == 1
        upsample_destroy();
#endif  // USE_DOWNSAMPLE == 1
    }

    native_playback_destroy();
}

void msbc_decoder_prepare_for_next(void)
{
    msbc_decoder_prep_for_next = true;
}

void msbc_decoder_recv_frame(pWORD8 buffer, UWORD32 length)
{
    struct task_msg_t *msg;
    int left_space = MSBC_DECODER_BUFFER_SIZE - msbc_decoder_buffer_input_index;

//    uart_putc_noint_no_wait('R');

    if(0) {
        pUWORD8 ptr = (pUWORD8)buffer;
        for(uint32_t i=0; i<length; i++) {
            printf("%02x", ptr[i]);
        }
        printf("\r\n");
    }

    /* check whether all request data have been received */
    if(msbc_decoder_request_data_size >= length) {
        msbc_decoder_request_data_size -= length;
        if(msbc_decoder_request_data_size == 0) {
            msbc_decoder_all_request_data_received = true;
        }
    }
    else {
        msbc_decoder_request_data_size = 0;
        msbc_decoder_all_request_data_received = true;
    }

    msbc_decoder_total_input += length;

    /* copy received data to local buffer */
    if(left_space <= length) {
        memcpy(msbc_decoder_buffer+msbc_decoder_buffer_input_index, buffer, left_space);
        length -= left_space;
        buffer += left_space;
        msbc_decoder_buffer_input_index = 0;
        if(length) {
            memcpy(msbc_decoder_buffer, buffer, length);
            msbc_decoder_buffer_input_index = length;
        }
    }
    else {
        memcpy(msbc_decoder_buffer+msbc_decoder_buffer_input_index, buffer, length);
        msbc_decoder_buffer_input_index += length;
    }

    MSBC_LOG("msbc_decoder_recv_frame: msbc_decoder_total_input = %d, msbc_decoder_buffer_input_index = %d.\r\n", msbc_decoder_total_input, msbc_decoder_buffer_input_index);
    MSBC_LOG_ERR("receive from mcu\r\n");

    if((msbc_decoder_state == MSBC_DECODER_IDLE) && (MSBC_DECODER_BUFFERING_LIMIT <= msbc_decoder_total_input)) {
        msbc_decoder_state = MSBC_DECODER_BUFFERING;
        msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
        task_msg_insert(msg);
    }
}
