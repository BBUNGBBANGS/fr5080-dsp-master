/*
 * sbc_encoder.c
 *
 *  Created on: 2018-9-20
 *      Author: owen
 */

#include <string.h>
#include <stdio.h>

#include <xtensa/xtruntime.h>

#include "co_mem.h"
#include "co_list.h"
#include "ipc.h"
#include "user_def.h"
#include "plf.h"

#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"
#include "xa_apicmd_standards.h"
#include "xa_memory_standards.h"
#include "msbc_enc/xa_msbc_enc_api.h"
#if __XTENSA_EB__
#include <xtensa/tie/xt_hifi2.h>
#endif

#ifdef __XCC__
#include <xtensa/hal.h>
#endif

#define SBC_ENCODER_LOG             //printf
#define SBC_ENCODER_ERROR           //printf

#define MSBC_FRAME_LENGTH           57
#define MSBC_SUB_FRAME_COUNT        4
#define MSBC_MAX_STORED_COUNT       400

#define MAX_MEM_ALLOCS              100

struct msbc_encoder_encoded_frame_t {
    struct co_list_hdr hdr;
    pUWORD8 data;
    UWORD32 length;
};

static pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
static WORD  g_w_malloc_count = 0;

/* The process API function */
static xa_codec_func_t *p_xa_process_api;
/* API obj */
static xa_codec_handle_t xa_process_handle;

/* Process initing done query variable */
static pWORD8 pb_inp_buf, pb_out_buf;
static UWORD32 ui_inp_size;
static WORD32 i_bytes_consumed, i_bytes_read;
static WORD32 i_buff_size;
static UWORD32 ui_init_done = 0;

/* used to store PCM data which have not been encoded */
static UWORD32 msbc_encoder_buffer_level = 0;

/* encoded SBC frame list */
static struct co_list msbc_encoder_encoded_frame_list;
static UWORD32 msbc_encoder_encoded_frame_count = 0;
static UWORD32 msbc_encoder_encoded_data_length = 0;

static bool msbc_encoder_ready = false;

static void msbc_encoder_send_next_frame_done(uint8_t channel);

static void *msbc_encoder_malloc(UWORD32 length)
{
    void *ptr = pvPortMalloc_user(length);

    if(ptr == NULL) {
        SBC_ENCODER_ERROR("sbc_encoder_malloc failed.\r\n");
    }
    //SBC_ENCODER_LOG("sbc_encoder_malloc: ptr=0x%08x, length=%d.\r\n", ptr, length);

    return ptr;
}

static void msbc_encoder_free(void *buffer)
{
    vPortFree_user((void *)buffer);
}

static XA_ERRORCODE xa_msbc_enc_get_config_param (xa_codec_handle_t p_xa_process_api_obj,
                                                    pWORD32           pi_bitrate)
{
    XA_ERRORCODE err_code = XA_NO_ERROR;

    /* Data rate */
    {
        err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                      XA_API_CMD_GET_CONFIG_PARAM,
                      XA_MSBC_ENC_CONFIG_PARAM_BITRATE, pi_bitrate);
        SBC_ENCODER_ERROR("xa_msbc_enc_get_config_param %04x.\r\n", err_code);
    }

    return XA_NO_ERROR;
}

static void msbc_encoder_engine_encode(void)
{
    /* Error code */
    XA_ERRORCODE err_code_exec = XA_NO_ERROR;
    XA_ERRORCODE err_code = XA_NO_ERROR;
    UWORD32 ui_exec_done;
    UWORD32 i_out_bytes;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_SET_INPUT_BYTES,
                                   0,
                                   &i_buff_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_encode at line %d.\r\n", __LINE__);
    }

    /* Execute process */
    err_code_exec = (*p_xa_process_api)(xa_process_handle,
                                        XA_API_CMD_EXECUTE,
                                        XA_CMD_TYPE_DO_EXECUTE,
                                        NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_encode at line %d.\r\n", __LINE__);
    }

    /* Checking for end of processing */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_EXECUTE,
                                   XA_CMD_TYPE_DONE_QUERY,
                                   &ui_exec_done);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_encode at line %d.\r\n", __LINE__);
    }

    /* Get the output bytes */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_OUTPUT_BYTES,
                                   0,
                                   &i_out_bytes);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_encode at line %d.\r\n", __LINE__);
    }

    //SBC_ENCODER_LOG("sbc_encoder_engine_encode: XA_API_CMD_GET_OUTPUT_BYTES out = %d.\r\n", i_out_bytes);
    if(i_out_bytes) {
        if(0) {
            pUWORD8 ptr = (pUWORD8)pb_out_buf;
            for(uint32_t i=0; i<i_out_bytes; i++) {
                printf("%02x", ptr[i]);
            }
            printf("\r\n");
        }
        else {
            SBC_ENCODER_LOG("msbc_encoder_engine_encode count = %d.\r\n", msbc_encoder_encoded_frame_count);
            if(msbc_encoder_encoded_frame_count >= MSBC_MAX_STORED_COUNT) {
                struct msbc_encoder_encoded_frame_t *frame;
                frame = (void *)co_list_pop_front(&msbc_encoder_encoded_frame_list);
                msbc_encoder_encoded_frame_count--;
                msbc_encoder_encoded_data_length -= frame->length;
                msbc_encoder_free(frame->data);
                msbc_encoder_free(frame);
            }
            struct msbc_encoder_encoded_frame_t *frame;
            frame = (void *)msbc_encoder_malloc(sizeof(struct msbc_encoder_encoded_frame_t));
            frame->data = msbc_encoder_malloc(i_out_bytes);
            memcpy(frame->data, pb_out_buf, i_out_bytes);
            frame->length = i_out_bytes;
            GLOBAL_INT_DISABLE();
            co_list_push_back(&msbc_encoder_encoded_frame_list, &frame->hdr);
            msbc_encoder_encoded_frame_count++;
            msbc_encoder_encoded_data_length += i_out_bytes;
            GLOBAL_INT_RESTORE();
        }
    }

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                                   0,
                                   &i_bytes_consumed);

    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_encode at line %d.\r\n", __LINE__);
    }

    printf("i_bytes_consumed = %d, i_out_bytes = %d.\r\n", i_bytes_consumed, i_out_bytes);
}

static UWORD32 msbc_encoder_engine_start(void)
{
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_SET_INPUT_BYTES,
                                   0,
                                   &i_buff_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_start at line %d.\r\n", __LINE__);
    }

    /* Initialize the process */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_INIT,
                                   XA_CMD_TYPE_INIT_PROCESS,
                                   NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_start at line %d.\r\n", __LINE__);
    }

    /* Checking for end of initialization */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_INIT,
                                   XA_CMD_TYPE_INIT_DONE_QUERY,
                                   &ui_init_done);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_start at line %d.\r\n", __LINE__);
    }

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                                   0,
                                   &i_bytes_consumed);

    SBC_ENCODER_LOG("msbc_encoder_engine_start: ui_init_done = %d.\r\n", ui_init_done);

    return ui_init_done;
}

void msbc_encoder_engine_init(UWORD32 i_bitrate)
{
    LOOPIDX i;

    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    XA_ERRORCODE err_code_exec = XA_NO_ERROR;

    /* Memory variables */
    UWORD32 n_mems;
    UWORD32 ui_proc_mem_tabs_size;
    /* API size */
    UWORD32 pui_api_size;

    /* Stack process struct initing */
    p_xa_process_api = xa_msbc_enc;
    /* Stack process struct initing end */

    /* ******************************************************************/
    /* Initialize API structure and set config params to default        */
    /* ******************************************************************/

    /* Get the API size */
    err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0,
                                    &pui_api_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
    }

    /* Allocate memory for API */
    g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_encoder_malloc(pui_api_size);

    /* Set API object with the memory allocated */
    xa_process_handle = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

    g_w_malloc_count++;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_INIT,
                                    XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                                    NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
    }

    /* ******************************************************************/
    /* Initialize Memory info tables                                    */
    /* ******************************************************************/

    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_GET_MEMTABS_SIZE, 0,
                                    &ui_proc_mem_tabs_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
    }

    g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_encoder_malloc(ui_proc_mem_tabs_size);

    /* Set pointer for process memory tables      */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_SET_MEMTABS_PTR, 0,
                                    (void *) g_pv_arr_alloc_memory[g_w_malloc_count]);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
    }

    g_w_malloc_count++;

    /* initialize the API, post config, fill memory tables        */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                    XA_API_CMD_INIT,
                                    XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
                                    NULL);

    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
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
        SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
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
            SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
        }

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                        XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
                                        i,
                                        &ui_alignment);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
        }

        /* Get memory type */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                        XA_API_CMD_GET_MEM_INFO_TYPE,
                                        i,
                                        &ui_type);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
        }

        g_pv_arr_alloc_memory[g_w_malloc_count] = msbc_encoder_malloc(ui_size);

        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
        }

        pv_alloc_ptr = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

        g_w_malloc_count++;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                        XA_API_CMD_SET_MEM_PTR,
                                        i,
                                        pv_alloc_ptr);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("msbc_encoder_engine_init at line %d.\r\n", __LINE__);
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

static void msbc_ipc_rx(void)
{
    uint8_t *mic;

    ipc_dma_rx_int_clear();

    if(ipc_get_codec_adc_tog() == 0) {
        mic = (uint8_t *)IPC_CODEC_READ_BUFFER1;
    }
    else {
        mic = (uint8_t *)IPC_CODEC_READ_BUFFER0;
    }

    msbc_encoder_recv_frame(mic, 120);
}

void msbc_encoder_init(UWORD32 i_bitrate)
{
    SBC_ENCODER_LOG("msbc_encoder_init: i_bitrate=%d\r\n", i_bitrate);
    ui_init_done = 0;

    msbc_encoder_engine_init(i_bitrate);

    co_list_init(&msbc_encoder_encoded_frame_list);
    msbc_encoder_encoded_frame_count = 0;
    msbc_encoder_encoded_data_length = 0;
    msbc_encoder_buffer_level = 0;

    msbc_encoder_ready = true;

    ipc_dma_init(msbc_ipc_rx, NULL);
    _xtos_interrupt_enable(XCHAL_IPC_DMA_RX_INTERRUPT);
}

void msbc_encoder_destroy(void)
{
    struct msbc_encoder_encoded_frame_t * frame;
    uint32_t i;

    GLOBAL_INT_DISABLE();
    while(msbc_encoder_encoded_frame_count) {
        msbc_encoder_encoded_frame_count--;
        frame = (struct msbc_encoder_encoded_frame_t *)co_list_pop_front(&msbc_encoder_encoded_frame_list);
        msbc_encoder_free(frame->data);
        msbc_encoder_free(frame);
    }
    msbc_encoder_encoded_data_length = 0;
    GLOBAL_INT_RESTORE();

    for(i=0; i<g_w_malloc_count; i++) {
        msbc_encoder_free(g_pv_arr_alloc_memory[i]);
    }
    g_w_malloc_count = 0;

    msbc_encoder_ready = false;
}

void msbc_encoder_recv_frame(pWORD8 buffer, UWORD32 length)
{
    UWORD32 frame_length, last_space;

    if(msbc_encoder_ready == false) {
        return;
    }

    while(length > 0) {
        last_space = ui_inp_size - msbc_encoder_buffer_level;
        if(length >= last_space) {
            memcpy(pb_inp_buf+msbc_encoder_buffer_level, buffer, last_space);
            length -= last_space;
            buffer += last_space;
            i_buff_size = ui_inp_size;
            if(ui_init_done == 0) {
                msbc_encoder_engine_start();
            }
            else {
                msbc_encoder_engine_encode();
            }
            last_space = i_buff_size - i_bytes_consumed;
            if(last_space) {
                memcpy(pb_inp_buf, pb_inp_buf+i_bytes_consumed, last_space);
                msbc_encoder_buffer_level = last_space;
            }
            else {
                msbc_encoder_buffer_level = 0;
            }

        }
        else {
            memcpy(pb_inp_buf+msbc_encoder_buffer_level, buffer, length);
            msbc_encoder_buffer_level += length;
            length = 0;
        }
    }

    SBC_ENCODER_LOG("msbc_encoder_recv_frame: sbc_encoder_buffer_level=%d.\r\n", msbc_encoder_buffer_level);
}

void msbc_encoder_recv_frame_req(void *arg)
{
    uint8_t left_frame_to_send = MSBC_SUB_FRAME_COUNT;
    uint8_t channel;
    UWORD32 length = 0;
    pUWORD8 buffer;
    pUWORD16 single_frame_len;

    if(msbc_encoder_ready == false) {
        return;
    }

    printf("sbc_req.\r\n");

    GLOBAL_INT_DISABLE();
    SBC_ENCODER_LOG("sbc_req before: %d.\r\n", msbc_encoder_encoded_frame_count);
    GLOBAL_INT_RESTORE();

    channel = ipc_alloc_channel(MSBC_FRAME_LENGTH * MSBC_SUB_FRAME_COUNT + 2);
    if(channel != 0xff) {
        buffer= ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
        single_frame_len =(pUWORD16)buffer;
        buffer += 2;
        length += 2;
        while((left_frame_to_send) && (msbc_encoder_encoded_frame_count)) {
            struct msbc_encoder_encoded_frame_t * frame;
            GLOBAL_INT_DISABLE();
            frame = (struct msbc_encoder_encoded_frame_t *)co_list_pop_front(&msbc_encoder_encoded_frame_list);
            msbc_encoder_encoded_data_length -= frame->length;
            msbc_encoder_encoded_frame_count--;
            GLOBAL_INT_RESTORE();
            memcpy(buffer, frame->data, frame->length);
            *single_frame_len = frame->length;
            length += frame->length;
            buffer += frame->length;
            vPortFree_user(frame->data);
            vPortFree_user(frame);
            left_frame_to_send--;
        }

        if(length > 2) {
            ipc_insert_msg(channel, IPC_MSG_ENCODED_MSBC_FRAME, length, ipc_free_channel);
        }
        else {
            // no more data to be send
            ipc_free_channel(channel);
        }
    }
    GLOBAL_INT_DISABLE();
    SBC_ENCODER_LOG("sbc_req after: %d.\r\n", msbc_encoder_encoded_frame_count);
    GLOBAL_INT_RESTORE();
}
