/*
 * sbc_encoder.c
 *
 *  Created on: 2018-9-20
 *      Author: owen
 */

#include <string.h>
#include <stdio.h>

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
#include "sbc_enc/xa_sbc_enc_api.h"
#if __XTENSA_EB__
#include <xtensa/tie/xt_hifi2.h>
#endif

#ifdef __XCC__
#include <xtensa/hal.h>
#endif

#define SBC_ENCODER_LOG             //printf
#define SBC_ENCODER_ERROR           //printf

#define SBC_A2DP_FRAME_LENGTH       300
#define SBC_A2DP_SUB_FRAME_COUNT    5

#define MAX_MEM_ALLOCS              100

struct sbc_encoder_encoded_frame_t {
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
static UWORD32 sbc_encoder_buffer_level = 0;

/* encoded SBC frame list */
static struct co_list sbc_encoder_encoded_frame_list;
static UWORD32 sbc_encoder_encoded_frame_count = 0;
static UWORD32 sbc_encoder_encoded_data_length = 0;

static bool sbc_encoder_prep_for_next = false;
static bool sbc_encoder_ready = false;

static void sbc_encoder_send_next_frame_done(uint8_t channel);

static void *sbc_encoder_malloc(UWORD32 length)
{
    void *ptr = pvPortMalloc_user(length);

    if(ptr == NULL) {
        SBC_ENCODER_ERROR("sbc_encoder_malloc failed.\r\n");
    }
    //SBC_ENCODER_LOG("sbc_encoder_malloc: ptr=0x%08x, length=%d.\r\n", ptr, length);

    return ptr;
}

static void sbc_encoder_free(void *buffer)
{
    vPortFree_user((void *)buffer);
}

static XA_ERRORCODE xa_sbc_enc_set_config_param (UWORD32            i_samp_freq,
                             UWORD32            i_num_chan,
                             UWORD32            i_subbands,
                             UWORD32            i_blocks,
                             UWORD32            i_snr,
                             UWORD32            i_bitpool,
                             xa_codec_handle_t  p_xa_process_api_obj)
{
    LOOPIDX i;
    XA_ERRORCODE err_code = XA_NO_ERROR;

    /* the process API function */
    xa_codec_func_t *p_xa_process_api = xa_sbc_enc;

    UWORD32 ui_chmode = XA_SBC_ENC_CHMODE_MONO;
    if (i_num_chan == 2)
        ui_chmode = XA_SBC_ENC_CHMODE_JOINT;

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_SAMP_FREQ, &i_samp_freq);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_SUBBANDS, &i_subbands);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_BLOCKS, &i_blocks);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_SNR, &i_snr);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_BITPOOL, &i_bitpool);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
                                    XA_API_CMD_SET_CONFIG_PARAM,
                                    XA_SBC_ENC_CONFIG_PARAM_CHMODE, &ui_chmode);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    return XA_NO_ERROR;
}

#if 0
void sbc_encoder_send_next_frame(void)
{
    struct sbc_encoder_encoded_frame_t *frame;

    frame = (struct sbc_encoder_encoded_frame_t *)co_list_pick(&sbc_encoder_encoded_frame_list);
    if(frame) {
        uint8_t channel;
        uint8_t *buffer;

        channel = ipc_alloc_channel(frame->length);
        if(channel != 0xff) {
            printf("sbc_encoder_send_next_frame: channel = %d, length = %d\r\n", channel, frame->length);
            buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
            memcpy(buffer, frame->data, frame->length);
            ipc_insert_msg(channel, IPC_MSG_ENCODED_SBC_FRAME, frame->length, sbc_encoder_send_next_frame_done);
            GLOBAL_INT_DISABLE();
            frame = (struct sbc_encoder_encoded_frame_t *)co_list_pop_front(&sbc_encoder_encoded_frame_list);
            GLOBAL_INT_RESTORE();
            vPortFree_user(frame->data);
            vPortFree_user(frame);
            if(co_list_is_empty(&sbc_encoder_encoded_frame_list) == 0) {
                struct task_msg_t *msg;
                msg = task_msg_alloc(SBC_ENCODER_TRANSMIT_NEXT_FRAME, 0);
                task_msg_insert(msg);
            }
        }
        else {
            struct task_msg_t *msg;
            msg = task_msg_alloc(SBC_ENCODER_TRANSMIT_NEXT_FRAME, 0);
            task_msg_insert(msg);
        }
    }
}

static void sbc_encoder_send_next_frame_done(uint8_t channel)
{
    ipc_free_channel(channel);
    printf("sbc_encoder_send_next_frame_done\r\n");
    sbc_encoder_send_next_frame();
}
#endif

static void sbc_encoder_engine_encode(void)
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
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Execute process */
    err_code_exec = (*p_xa_process_api)(xa_process_handle,
                                        XA_API_CMD_EXECUTE,
                                        XA_CMD_TYPE_DO_EXECUTE,
                                        NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Checking for end of processing */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_EXECUTE,
                                   XA_CMD_TYPE_DONE_QUERY,
                                   &ui_exec_done);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Get the output bytes */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_OUTPUT_BYTES,
                                   0,
                                   &i_out_bytes);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
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
#if 1
            struct sbc_encoder_encoded_frame_t *frame;
            frame = (void *)sbc_encoder_malloc(sizeof(struct sbc_encoder_encoded_frame_t));
            frame->data = sbc_encoder_malloc(i_out_bytes);
            memcpy(frame->data, pb_out_buf, i_out_bytes);
            frame->length = i_out_bytes;
            GLOBAL_INT_DISABLE();
            co_list_push_back(&sbc_encoder_encoded_frame_list, &frame->hdr);
            sbc_encoder_encoded_frame_count++;
            sbc_encoder_encoded_data_length += i_out_bytes;
            GLOBAL_INT_RESTORE();
#else
            struct sbc_encoder_encoded_frame_t *frame;
            frame = (void *)sbc_encoder_malloc(sizeof(struct sbc_encoder_encoded_frame_t));
            frame->data = sbc_encoder_malloc(i_out_bytes);
            memcpy(frame->data, pb_out_buf, i_out_bytes);
            frame->length = i_out_bytes;

            printf("sbc_encoder_engine_encode: ");
            GLOBAL_INT_DISABLE();
            if(co_list_is_empty(&sbc_encoder_encoded_frame_list)) {
                printf("1 ");
                struct task_msg_t *msg;
                msg = task_msg_alloc(SBC_ENCODER_TRANSMIT_NEXT_FRAME, 0);
                task_msg_insert(msg);
            }
            printf("\r\n");

            co_list_push_back(&sbc_encoder_encoded_frame_list, &frame->next);
            GLOBAL_INT_RESTORE();
#endif
        }
    }

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                                   0,
                                   &i_bytes_consumed);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }
    //SBC_ENCODER_LOG("sbc_encoder_engine_encode: XA_API_CMD_GET_CURIDX_INPUT_BUF consumed = %d.\r\n", i_bytes_consumed);
}

static UWORD32 sbc_encoder_engine_start(void)
{
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_SET_INPUT_BYTES,
                                   0,
                                   &i_buff_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Initialize the process */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_INIT,
                                   XA_CMD_TYPE_INIT_PROCESS,
                                   NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Checking for end of initialization */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_INIT,
                                   XA_CMD_TYPE_INIT_DONE_QUERY,
                                   &ui_init_done);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                   XA_API_CMD_GET_CURIDX_INPUT_BUF,
                                   0,
                                   &i_bytes_consumed);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }
    //SBC_ENCODER_LOG("sbc_encoder_engine_start: XA_API_CMD_GET_CURIDX_INPUT_BUF consumed = %d.\r\n", i_bytes_consumed);

    return ui_init_done;
}

void sbc_encoder_engine_init(UWORD32 i_samp_freq,
                                UWORD32 i_num_chan,
                                UWORD32 i_subbands,
                                UWORD32 i_blocks,
                                UWORD32 i_snr,
                                UWORD32 i_bitpool)
{
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    XA_ERRORCODE err_code_exec = XA_NO_ERROR;
    UWORD32 ui_api_size, ui_memtab_size, n_mems, i;

    /* Stack process struct initing */
    p_xa_process_api = xa_sbc_enc;

    /* ******************************************************************/
    /* Initialize API structure and set config params to default        */
    /* ******************************************************************/

    /* Get the API size */
    err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0,
                                    &ui_api_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* Allocate memory for API */
    g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_encoder_malloc(ui_api_size);

    /* Set API object with the memory allocated */
    xa_process_handle = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];
    g_w_malloc_count++;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                 XA_API_CMD_INIT,
                                 XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                                 NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* ******************************************************************/
    /* Set config parameters got from the user present in argc argv     */
    /* ******************************************************************/

    err_code = xa_sbc_enc_set_config_param(i_samp_freq,
                                            i_num_chan,
                                            i_subbands,
                                            i_blocks,
                                            i_snr,
                                            i_bitpool,
                                            xa_process_handle);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }

    /* ******************************************************************/
    /* Initialize Memory info tables                                    */
    /* ******************************************************************/

    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                 XA_API_CMD_GET_MEMTABS_SIZE, 0,
                                 &ui_memtab_size);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }
    g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_encoder_malloc(ui_memtab_size);

    /* Set pointer for process memory tables      */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                 XA_API_CMD_SET_MEMTABS_PTR, 0,
                                 (void *) g_pv_arr_alloc_memory[g_w_malloc_count]);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
    }
    g_w_malloc_count++;

    /* initialize the API, post config, fill memory tables        */
    err_code = (*p_xa_process_api)(xa_process_handle,
                                 XA_API_CMD_INIT,
                                 XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
                                 NULL);
    if(err_code != XA_NO_ERROR) {
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
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
        SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
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
            SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
        }

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                       XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
                                       i,
                                       &ui_alignment);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
        }

        /* Get memory type */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                       XA_API_CMD_GET_MEM_INFO_TYPE,
                                       i,
                                       &ui_type);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
        }

        g_pv_arr_alloc_memory[g_w_malloc_count] = sbc_encoder_malloc(ui_size);
        pv_alloc_ptr = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];
        g_w_malloc_count++;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(xa_process_handle,
                                       XA_API_CMD_SET_MEM_PTR,
                                       i,
                                       pv_alloc_ptr);
        if(err_code != XA_NO_ERROR) {
            SBC_ENCODER_ERROR("sbc encoder meets error at line %d.\r\n", __LINE__);
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

void sbc_encoder_init(UWORD32 i_samp_freq,
                                UWORD32 i_num_chan,
                                UWORD32 i_subbands,
                                UWORD32 i_blocks,
                                UWORD32 i_snr,
                                UWORD32 i_bitpool)
{
    SBC_ENCODER_LOG("sbc_encoder_init: freq=%d, chan=%d, subbands=%d, blocks=%d, snr=%d, bitpool=%d\r\n", i_samp_freq, i_num_chan, i_subbands, i_blocks, i_snr, i_bitpool);
    ui_init_done = 0;

    sbc_encoder_engine_init(i_samp_freq, i_num_chan, i_subbands, i_blocks, i_snr, i_bitpool);

    co_list_init(&sbc_encoder_encoded_frame_list);
    sbc_encoder_encoded_frame_count = 0;
    sbc_encoder_encoded_data_length = 0;
    sbc_encoder_buffer_level = 0;

    sbc_encoder_prep_for_next = false;
    sbc_encoder_ready = true;
}

void sbc_encoder_destroy(void)
{
    struct sbc_encoder_encoded_frame_t * frame;
    uint32_t i;

    GLOBAL_INT_DISABLE();
    while(sbc_encoder_encoded_frame_count) {
        sbc_encoder_encoded_frame_count--;
        frame = (struct sbc_encoder_encoded_frame_t *)co_list_pop_front(&sbc_encoder_encoded_frame_list);
        sbc_encoder_free(frame->data);
        sbc_encoder_free(frame);
    }
    sbc_encoder_encoded_data_length = 0;
    GLOBAL_INT_RESTORE();

    for(i=0; i<g_w_malloc_count; i++) {
        sbc_encoder_free(g_pv_arr_alloc_memory[i]);
    }
    g_w_malloc_count = 0;

    sbc_encoder_ready = false;
}

void sbc_encoder_prepare_for_next(void)
{
    sbc_encoder_prep_for_next = true;
}

void sbc_encoder_recv_frame(pWORD8 buffer, UWORD32 length)
{
    UWORD32 frame_length, last_space;

    if(sbc_encoder_ready == false) {
        return;
    }

    while(length > 0) {
        last_space = ui_inp_size - sbc_encoder_buffer_level;
        if(length >= last_space) {
            memcpy(pb_inp_buf+sbc_encoder_buffer_level, buffer, last_space);
            length -= last_space;
            buffer += last_space;
            i_buff_size = ui_inp_size;
            if(ui_init_done == 0) {
                sbc_encoder_engine_start();
            }
            else {
                sbc_encoder_engine_encode();
            }
            last_space = i_buff_size - i_bytes_consumed;
            if(last_space) {
                memcpy(pb_inp_buf, pb_inp_buf+i_bytes_consumed, last_space);
                sbc_encoder_buffer_level = last_space;
            }
            else {
                sbc_encoder_buffer_level = 0;
            }

        }
        else {
            memcpy(pb_inp_buf+sbc_encoder_buffer_level, buffer, length);
            sbc_encoder_buffer_level += length;
            length = 0;
        }
    }

    SBC_ENCODER_LOG("sbc_encoder_recv_frame: end with sbc_encoder_buffer_level=%d.\r\n", sbc_encoder_buffer_level);
}

void sbc_encoder_recv_frame_req(void *arg)
{
    uint8_t left_frame_to_send = SBC_A2DP_SUB_FRAME_COUNT;
    uint8_t channel;
    UWORD32 length = 0;
    pUWORD8 buffer;
    pUWORD16 single_frame_len;

    if(sbc_encoder_ready == false) {
        return;
    }

    GLOBAL_INT_DISABLE();
    SBC_ENCODER_LOG("sbc_req before: %d.\r\n", sbc_encoder_encoded_frame_count);
    GLOBAL_INT_RESTORE();

    channel = ipc_alloc_channel(SBC_A2DP_FRAME_LENGTH);
    if(channel != 0xff) {
        buffer= ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
        single_frame_len =(pUWORD16)buffer;
        buffer += 2;
        length += 2;
        while((left_frame_to_send) && (sbc_encoder_encoded_frame_count)) {
            struct sbc_encoder_encoded_frame_t * frame;
            GLOBAL_INT_DISABLE();
            frame = (struct sbc_encoder_encoded_frame_t *)co_list_pop_front(&sbc_encoder_encoded_frame_list);
            sbc_encoder_encoded_data_length -= frame->length;
            sbc_encoder_encoded_frame_count--;
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
            ipc_insert_msg(channel, IPC_MSG_ENCODED_SBC_FRAME, length, ipc_free_channel);
        }
        else {
        	// no more data to be send
            ipc_free_channel(channel);

            /* prepare for next song when sending list is empty */
            if(sbc_encoder_prep_for_next) {
                sbc_encoder_ready = false;
                struct task_msg_t *msg;
                msg = task_msg_alloc(DECODER_PREPARE_FOR_NEXT, 0);
                task_msg_insert(msg);
            }
        }
    }
    GLOBAL_INT_DISABLE();
    SBC_ENCODER_LOG("sbc_req after: %d.\r\n", sbc_encoder_encoded_frame_count);
    GLOBAL_INT_RESTORE();

    /* sbc_encoder_ready is true means mp3 decoder is ready too */
    if(sbc_encoder_ready) {
        if(sbc_encoder_encoded_frame_count < SBC_A2DP_SUB_FRAME_COUNT) {
            struct task_msg_t *msg;
            msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
            task_msg_insert(msg);
        }
    }
}
