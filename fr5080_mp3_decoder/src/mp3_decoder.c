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
#include "mp3_dec/xa_mp3_dec_api.h"

#include "tasks.h"
#include "ipc.h"
#include "co_mem.h"
#include "plf.h"
#include "user_def.h"
#include "upsample.h"
#include "downsample.h"

#define USE_DOWNSAMPLE                  1

#define MP3_LOG                         //printf
#define MP3_LOG_ERR                     //printf

#define MP3_DECODED_TO_SBC_ENCODER      1

#define IPC_MAX_TRANSMIT_SIZE           512

#define MAX_MEM_ALLOCS                  100

/* send IPC_MSG_RAW_BUFFER_SPACE to request more data after MP3_DECODER_REQUEST_DATA_THD bytes have been consumed */
#define MP3_DECODER_REQUEST_DATA_THD    1024

/* MP3 decoder input buffer size */
#define MP3_DECODER_BUFFER_SIZE         8192
/* Start decoder after how many bytes have been received */
#define MP3_DECODER_BUFFERING_LIMIT     2048
/* indicate the limitation of apply size */
#define MP3_DECODER_APPLY_LIMIT         4096

enum mp3_decoder_state_t {
    MP3_DECODER_IDLE,
    MP3_DECODER_BUFFERING,
    MP3_DECODER_INITIATING,
    MP3_DECODER_DECODING,
    MP3_DECODER_PCM_TRANSMITTING,
};

struct mp3_pcm_msg_t {
    pUWORD8 ptr;
    UWORD32 last_length;
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
static pWORD8 mp3_decoder_buffer = NULL;
static UWORD32 mp3_decoder_buffer_input_index = 0;
static UWORD32 mp3_decoder_buffer_output_index = 0;
static UWORD32 mp3_decoder_total_input = 0;

static pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
static WORD  g_w_malloc_count = 0;

/* how many bytes have been consumed since IPC_MSG_RAW_BUFFER_SPACE is sent */
static UWORD32 mp3_decoder_dynamic_consumed = 0;
static bool mp3_decoder_all_request_data_received = false;
static UWORD32 mp3_decoder_request_data_size = 0;
static bool mp3_decoder_prep_for_next = false;

static enum mp3_decoder_state_t mp3_decoder_state = MP3_DECODER_IDLE;
static struct mp3_pcm_msg_t mp3_pcm_msg;

struct sbc_info_t sbc_info;

static bool resample_used = false;

void sbc_encoder_init(UWORD32 i_samp_freq,
                                UWORD32 i_num_chan,
                                UWORD32 i_subbands,
                                UWORD32 i_blocks,
                                UWORD32 i_snr,
                                UWORD32 i_bitpool);
void sbc_encoder_recv_frame(pWORD8 buffer, UWORD32 length);

static void *mp3_mem_alloc(UWORD32 length)
{
    void *ptr = pvPortMalloc_user(length);
    if(ptr == NULL) {
    	MP3_LOG_ERR("mp3_mem_alloc: failed.\r\n");
    }

    MP3_LOG("mp3_mem_alloc: ptr=0x%08x, length=%d.\r\n", ptr, length);

    return ptr;
}

static void mp3_mem_free(void *buffer)
{
	vPortFree_user((void *)buffer);
}

static WORD32 xa_mp3_dec_get_data(pVOID buffer, WORD32 size)
{
    int length, deal_length;

    GLOBAL_INT_DISABLE();
    MP3_LOG("xa_mp3_dec_get_data start: mp3_decoder_buffer_output_index=%d.\r\n", mp3_decoder_buffer_output_index);
    if(mp3_decoder_buffer_output_index < mp3_decoder_buffer_input_index) {
        length = mp3_decoder_buffer_input_index - mp3_decoder_buffer_output_index;
        if(length < size) {
            size = length;
        }
        memcpy(buffer, &mp3_decoder_buffer[mp3_decoder_buffer_output_index], size);
        mp3_decoder_buffer_output_index += size;

        deal_length = size;
    }
    else if(mp3_decoder_buffer_output_index == mp3_decoder_buffer_input_index) {
        deal_length = 0;
    }
    else {
        length = MP3_DECODER_BUFFER_SIZE - mp3_decoder_buffer_output_index;
        if(length >= size) {
            length = size;
        }
        memcpy(buffer, &mp3_decoder_buffer[mp3_decoder_buffer_output_index], length);
        mp3_decoder_buffer_output_index += length;
        if(mp3_decoder_buffer_output_index == MP3_DECODER_BUFFER_SIZE) {
            mp3_decoder_buffer_output_index = 0;
        }
        deal_length = length;
        if(length != size) {
            size -= length;
            deal_length += xa_mp3_dec_get_data((uint8_t *)buffer+deal_length, size);
        }
    }
    MP3_LOG("xa_mp3_dec_get_data end: mp3_decoder_buffer_output_index=%d.\r\n", mp3_decoder_buffer_output_index);
    GLOBAL_INT_RESTORE();

    return deal_length;
}

static void xa_shift_input_buffer (char *buf, int buf_size, int bytes_consumed)
{
    int i;

    if (bytes_consumed <= 0)
        return;

    #if __XCC__
    /* Optimize 2-byte aligned data movement. */
    if ((((int)buf | buf_size | bytes_consumed) & 1) == 0)
    {
        /* Optimize 4-byte aligned data movement. */
        if ((((int)buf | buf_size | bytes_consumed) & 2) == 0)
        {
            ae_p16x2s *dst = (ae_p16x2s *)buf;
            ae_p16x2s *src = (ae_p16x2s *)&buf[bytes_consumed];
            for (i = 0; i < (buf_size - bytes_consumed) >> 2; i++)
            {
                dst[i] = src[i];
            }
            return;
        }

        ae_p16s *dst = (ae_p16s *)buf;
        ae_p16s *src = (ae_p16s *)&buf[bytes_consumed];
        for (i = 0; i < (buf_size - bytes_consumed) >> 1; i++)
        {
            dst[i] = src[i];
        }
        return;
    }
    #endif

    /* Default, non-aligned data movement. */
    for (i = 0; i < buf_size - bytes_consumed; i++)
    {
        buf[i] = buf[i + bytes_consumed];
    }
}

#if 0
static void mp3_decoder_transmit_next(void)
{
    UWORD32 frame_size;
    uint8_t channel;
    uint8_t *buffer;

    /* 播放被暂停或者重新初始化，可能会修改这个状态 */
    if(mp3_decoder_state != MP3_DECODER_PCM_TRANSMITTING) {
        return;
    }

    frame_size = mp3_pcm_msg.last_length;
    if(frame_size > IPC_MAX_TRANSMIT_SIZE) {
        frame_size = IPC_MAX_TRANSMIT_SIZE;
    }

    channel = ipc_alloc_channel(frame_size);
    if(channel != 0xff) {
        buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
        memcpy(buffer, mp3_pcm_msg.ptr, frame_size);
        if(frame_size < mp3_pcm_msg.last_length) {
            mp3_pcm_msg.ptr += frame_size;
            mp3_pcm_msg.last_length -= frame_size;
            ipc_insert_msg(channel, 4/*IPC_MSG_DECODED_FRAME*/, frame_size, mp3_decoder_transmit_frame_done);
        }
        else {
            mp3_decoder_state = MP3_DECODER_DECODING;
            ipc_insert_msg(channel, 4/*IPC_MSG_DECODED_FRAME*/, frame_size, ipc_free_channel);

            struct task_msg_t *msg;
            msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
            task_msg_insert(msg);
        }
    }
    else {
        struct task_msg_t *msg;
        msg = task_msg_alloc(MP3_DECODER_TRANSMIT_NEXT_PCM, 0);
        task_msg_insert(msg);
    }
}

static void mp3_decoder_transmit_frame_done(uint8_t channel)
{
    ipc_free_channel(channel);

    mp3_decoder_transmit_next();
}
#endif

static void mp3_decoder_engine_decode(void)
{
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    XA_ERRORCODE err_code_exec = XA_NO_ERROR;
    UWORD32 left_data_length, i_out_bytes;

    left_data_length = i_buff_size - i_bytes_consumed;
    if((left_data_length != 0) && (i_bytes_consumed != 0)) {
        memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], left_data_length);
        //xa_shift_input_buffer(pb_inp_buf, i_buff_size, i_bytes_consumed);
    }
    i_bytes_read = xa_mp3_dec_get_data(pb_inp_buf + left_data_length,
                                            ui_inp_size - left_data_length);
    i_buff_size = left_data_length + i_bytes_read;

    if((i_bytes_consumed == 0) && (i_bytes_read == 0)) {
        return;
    }
    i_bytes_consumed = 0;

    uart_putc_noint('<');

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_SET_INPUT_BYTES,
        0,
        &i_buff_size);
    MP3_LOG("mp3_decoder_engine_decode: XA_API_CMD_SET_INPUT_BYTES--err%04x, i_buff_size=%d.\r\n", err_code, i_buff_size);
    if(err_code != 0) {
        MP3_LOG_ERR("mp3_decoder meets error at line: %d.\r\n", __LINE__);
    }

    /* Execute process */
    err_code_exec = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_EXECUTE,
        XA_CMD_TYPE_DO_EXECUTE,
        NULL);
    MP3_LOG("mp3_decoder_engine_decode: XA_CMD_TYPE_DO_EXECUTE--err%04x.\r\n", err_code);
    if(err_code != 0) {
        MP3_LOG_ERR("mp3_decoder meets error at line: %d.\r\n", __LINE__);
        goto error_exit;
    }

    /* Checking for end of processing */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_EXECUTE,
        XA_CMD_TYPE_DONE_QUERY,
        &ui_exec_done);
    MP3_LOG("mp3_decoder_engine_decode: XA_CMD_TYPE_DONE_QUERY--err%04x, ui_exec_done=%d.\r\n", err_code, ui_exec_done);
    if(err_code != 0) {
        MP3_LOG_ERR("mp3_decoder meets error at line: %d.\r\n", __LINE__);
    }

    /* Get the output bytes */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_OUTPUT_BYTES,
        0,
        &i_out_bytes);
    MP3_LOG("mp3_decoder_engine_decode: XA_API_CMD_GET_OUTPUT_BYTES--err%04x, i_out_bytes=%d.\r\n", err_code, i_out_bytes);
    if(err_code != 0) {
        MP3_LOG_ERR("mp3_decoder meets error at line: %d.\r\n", __LINE__);
    }

    if(i_out_bytes) {
#if MP3_DECODED_TO_SBC_ENCODER == 1
        if(resample_used) {
            uart_putc_noint('[');
#if USE_DOWNSAMPLE == 1
            downsample_entity(pb_out_buf, i_out_bytes, sbc_encoder_recv_frame);
#else
            upsample_entity(pb_out_buf, i_out_bytes, sbc_encoder_recv_frame);
#endif
            uart_putc_noint(']');
        }
        else {
            sbc_encoder_recv_frame(pb_out_buf, i_out_bytes);
        }

        //struct task_msg_t *msg;
        //msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
        //task_msg_insert(msg);
#else // MP3_DECODED_TO_SBC_ENCODER == 1
        uint8_t channel;
        uint8_t *buffer;
        UWORD32 frame_size = i_out_bytes;

        if(i_out_bytes > IPC_MAX_TRANSMIT_SIZE) {
            frame_size = IPC_MAX_TRANSMIT_SIZE;
        }
        channel = ipc_alloc_channel(frame_size);
        if(channel != 0xff) {
            buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, channel);
            memcpy(buffer, pb_out_buf, frame_size);
            if(frame_size < i_out_bytes) {
                mp3_pcm_msg.ptr = pb_out_buf + frame_size;
                mp3_pcm_msg.last_length = i_out_bytes - frame_size;
                mp3_decoder_state = MP3_DECODER_PCM_TRANSMITTING;
                ipc_insert_msg(channel, IPC_MSG_DECODED_PCM_FRAME, frame_size, mp3_decoder_transmit_frame_done);
            }
            else {
                ipc_insert_msg(channel, IPC_MSG_DECODED_PCM_FRAME, frame_size, ipc_free_channel);

                struct task_msg_t *msg;
                msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
                task_msg_insert(msg);
            }
        }
        else {
            struct task_msg_t *msg;

            mp3_pcm_msg.ptr = pb_out_buf;
            mp3_pcm_msg.last_length = i_out_bytes;
            mp3_decoder_state = MP3_DECODER_PCM_TRANSMITTING;

            msg = task_msg_alloc(MP3_DECODER_TRANSMIT_NEXT_PCM, 0);
            task_msg_insert(msg);
        }
#endif  // MP3_DECODED_TO_SBC_ENCODER == 1
    }

error_exit:
    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_CURIDX_INPUT_BUF,
        0,
        &i_bytes_consumed);
    MP3_LOG("mp3_decoder_engine_decode: XA_API_CMD_GET_CURIDX_INPUT_BUF--err%04x, i_bytes_consumed=%d.\r\n", err_code, i_bytes_consumed);

    mp3_decoder_dynamic_consumed += i_bytes_consumed;

    MP3_LOG("mp3_decoder_dynamic_consumed is %d.\r\n", mp3_decoder_dynamic_consumed);
    /* apply more raw data after more than MP3_DECODER_REQUEST_DATA_THD bytes have been consumed */
    if((mp3_decoder_all_request_data_received == true)
    	&& (mp3_decoder_dynamic_consumed >= MP3_DECODER_REQUEST_DATA_THD)
        && (mp3_decoder_prep_for_next == false)) {
        uint8_t channel = ipc_alloc_channel(0);
        UWORD32 left_space;

        if(channel != 0xff) {
            MP3_LOG("apply more data, %d, %d.\r\n", mp3_decoder_buffer_output_index, mp3_decoder_buffer_input_index);
            GLOBAL_INT_DISABLE();
            if(mp3_decoder_buffer_output_index > mp3_decoder_buffer_input_index) {
                left_space = mp3_decoder_buffer_input_index + MP3_DECODER_BUFFER_SIZE - mp3_decoder_buffer_output_index;
            }
            else {
                left_space = mp3_decoder_buffer_input_index - mp3_decoder_buffer_output_index;
            }
            GLOBAL_INT_RESTORE();
            uint32_t apply_unit;
            if((MP3_DECODER_BUFFER_SIZE-left_space) > MP3_DECODER_APPLY_LIMIT) {
                apply_unit = MP3_DECODER_APPLY_LIMIT/256 - 1;
            }
            else {
                apply_unit = (MP3_DECODER_BUFFER_SIZE-left_space)/256 - 1;
            }
            if(apply_unit) {
                MP3_LOG("apply more data, %d.\r\n", apply_unit);
                ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);
                mp3_decoder_dynamic_consumed = 0;

                mp3_decoder_all_request_data_received = false;
                mp3_decoder_request_data_size = apply_unit*256;
            }
            else {
                ipc_free_channel(channel);
            }
        }
    }

    uart_putc_noint('>');
}

static WORD8 mp3_decoder_engine_start(void)
{
    UWORD32 left_data_length;
    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;

    left_data_length = i_buff_size - i_bytes_consumed;
    memcpy(pb_inp_buf, &pb_inp_buf[i_bytes_consumed], left_data_length);
    i_bytes_read = xa_mp3_dec_get_data(pb_inp_buf + left_data_length,
                                            ui_inp_size-left_data_length);
    i_buff_size = left_data_length + i_bytes_read;

    /* Set number of bytes to be processed */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_SET_INPUT_BYTES,
        0,
        &i_buff_size);
    MP3_LOG("mp3_decoder_engine_start: XA_API_CMD_SET_INPUT_BYTES--err%04x, i_buff_size=%d.\r\n", err_code, i_buff_size);
    if(err_code != XA_NO_ERROR) {
    	goto error_exit;
    }

    /* Initialize the process */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_INIT,
        XA_CMD_TYPE_INIT_PROCESS,
        NULL);
    MP3_LOG("mp3_decoder_engine_start: XA_CMD_TYPE_INIT_PROCESS--err%04x.\r\n", err_code);
    if(err_code != XA_NO_ERROR) {
		goto error_exit;
	}

    /* Checking for end of initialization */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_INIT,
        XA_CMD_TYPE_INIT_DONE_QUERY,
        &ui_init_done);
    MP3_LOG("mp3_decoder_engine_start: XA_CMD_TYPE_INIT_DONE_QUERY--err%04x, init_done=%d.\r\n", err_code, ui_init_done);
    if(err_code != XA_NO_ERROR) {
		goto error_exit;
	}

    if(ui_init_done) {
#if MP3_DECODED_TO_SBC_ENCODER == 1
        UWORD32 i_samp_freq, i_num_chan;
        /* Sampling frequency */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_GET_CONFIG_PARAM,
            XA_MP3DEC_CONFIG_PARAM_SAMP_FREQ, &i_samp_freq);

        /* Number of channels */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_GET_CONFIG_PARAM,
            XA_MP3DEC_CONFIG_PARAM_NUM_CHANNELS, &i_num_chan);

#if USE_DOWNSAMPLE == 1
        sbc_encoder_init(44100, i_num_chan, sbc_info.sub_bands, sbc_info.blocks, 1, sbc_info.bit_pool);

        if(i_samp_freq == 48000) {
            resample_used = true;
            uart_putc_noint('{');
            downsample_init();
            uart_putc_noint('}');
        }
        else {
            resample_used = false;
        }
#else   // USE_DOWNSAMPLE == 1
        sbc_encoder_init(48000, i_num_chan, sbc_info.sub_bands, sbc_info.blocks, 1, sbc_info.bit_pool);

        if(i_samp_freq == 44100) {
            resample_used = true;
            uart_putc_noint('{');
            upsample_init();
            uart_putc_noint('}');
        }
        else {
            resample_used = false;
        }
#endif  // USE_DOWNSAMPLE == 1
#endif  // MP3_DECODED_TO_SBC_ENCODER == 1
    }

error_exit:
    /* How much buffer is used in input buffers */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_CURIDX_INPUT_BUF,
        0,
        &i_bytes_consumed);
    MP3_LOG("mp3_decoder_engine_start: XA_API_CMD_GET_CURIDX_INPUT_BUF--err%04x, i_bytes_consumed=%d.\r\n", err_code, i_bytes_consumed);

    mp3_decoder_dynamic_consumed += i_bytes_consumed;

    /* apply more raw data after more than MP3_DECODER_REQUEST_DATA_THD bytes have been consumed */
    if((mp3_decoder_all_request_data_received == true)
        && (mp3_decoder_dynamic_consumed >= MP3_DECODER_REQUEST_DATA_THD)
        && (mp3_decoder_prep_for_next == false)) {
        uint8_t channel = ipc_alloc_channel(0);
        UWORD32 left_space;

        if(channel != 0xff) {
            MP3_LOG("apply more data, %d, %d.\r\n", mp3_decoder_buffer_output_index, mp3_decoder_buffer_input_index);
            GLOBAL_INT_DISABLE();
            if(mp3_decoder_buffer_output_index > mp3_decoder_buffer_input_index) {
                left_space = mp3_decoder_buffer_input_index + MP3_DECODER_BUFFER_SIZE - mp3_decoder_buffer_output_index;
            }
            else {
                left_space = mp3_decoder_buffer_input_index - mp3_decoder_buffer_output_index;
            }
            GLOBAL_INT_RESTORE();
            uint32_t apply_unit;
            if((MP3_DECODER_BUFFER_SIZE-left_space) > MP3_DECODER_APPLY_LIMIT) {
                apply_unit = MP3_DECODER_APPLY_LIMIT/256 - 1;
            }
            else {
                apply_unit = (MP3_DECODER_BUFFER_SIZE-left_space)/256 - 1;
            }
            if(apply_unit) {
                MP3_LOG("apply more data, %d.\r\n", apply_unit);
                ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);
                mp3_decoder_dynamic_consumed = 0;

                mp3_decoder_all_request_data_received = false;
                mp3_decoder_request_data_size = apply_unit*256;
            }
            else {
                ipc_free_channel(channel);
            }
        }
    }

    return ui_init_done;
}

static void mp3_decoder_engine_init(void)
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
    p_xa_process_api = xa_mp3_dec;

    /* Get the API size */
    err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0,
        &pui_api_size);
    MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_API_SIZE--err%04x.\r\n", err_code);

    /* Allocate memory for API */
    g_pv_arr_alloc_memory[g_w_malloc_count] = mp3_mem_alloc(pui_api_size);
    /* Set API object with the memory allocated */
    xa_process_handle = (xa_codec_handle_t) g_pv_arr_alloc_memory[g_w_malloc_count];
    g_w_malloc_count++;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_INIT,
        XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
        NULL);
    MP3_LOG("mp3_decoder_engine_init: XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS--err%04x.\r\n", err_code);

    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_MEMTABS_SIZE, 0,
        &ui_proc_mem_tabs_size);
    MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_MEMTABS_SIZE--err%04x, ui_proc_mem_tabs_size=%d.\r\n", err_code, ui_proc_mem_tabs_size);

    g_pv_arr_alloc_memory[g_w_malloc_count] = mp3_mem_alloc(ui_proc_mem_tabs_size);
    /* Set pointer for process memory tables  */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_SET_MEMTABS_PTR, 0,
        (void *) g_pv_arr_alloc_memory[g_w_malloc_count]);
    MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_SET_MEMTABS_PTR--err%04x.\r\n", err_code);
    g_w_malloc_count++;

    /* initialize the API, post config, fill memory tables  */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_INIT,
        XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
        NULL);
    MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_INIT--err%04x.\r\n", err_code);

    /* Get number of memory tables required */
    err_code = (*p_xa_process_api)(xa_process_handle,
        XA_API_CMD_GET_N_MEMTABS,
        0,
        &n_mems);
    MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_N_MEMTABS--err%04x.\r\n", err_code);

    for (i = 0; i < (WORD32) n_mems; i++) {
        int ui_size, ui_alignment, ui_type;
        pVOID pv_alloc_ptr;

        /* Get memory size */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_GET_MEM_INFO_SIZE,
            i,
            &ui_size);
        MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_MEM_INFO_SIZE--err%04x, ui_size=%d.\r\n", err_code, ui_size);

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
            i,
            &ui_alignment);
        MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_MEM_INFO_ALIGNMENT--err%04x.\r\n", err_code);

        /* Get memory type */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_GET_MEM_INFO_TYPE,
            i,
            &ui_type);
        MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_GET_MEM_INFO_TYPE--err%04x, ui_type=%d.\r\n", err_code, ui_type);

        g_pv_arr_alloc_memory[g_w_malloc_count] = mp3_mem_alloc(ui_size);
        pv_alloc_ptr = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];
        g_w_malloc_count++;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(xa_process_handle,
            XA_API_CMD_SET_MEM_PTR,
            i,
            pv_alloc_ptr);
        MP3_LOG("mp3_decoder_engine_init: XA_API_CMD_SET_MEM_PTR--err%04x.\r\n", err_code);

        if(ui_type == XA_MEMTYPE_INPUT) {
            pb_inp_buf = (pWORD8) pv_alloc_ptr;
            ui_inp_size = ui_size;
            i_buff_size = ui_inp_size;
            i_bytes_consumed = ui_inp_size;
        }
        if(ui_type == XA_MEMTYPE_OUTPUT) {
            pb_out_buf = (pWORD8) pv_alloc_ptr;
        }
    }

    memset((void *)&mp3_pcm_msg, 0, sizeof(mp3_pcm_msg));
}

static const char *mp3_decoder_state_str[] = {
    "MP3_DECODER_IDLE",
    "MP3_DECODER_BUFFERING",
    "MP3_DECODER_INITIATING",
    "MP3_DECODER_DECODING",
    "MP3_DECODER_PCM_TRANSMITTING",
};

void mp3_decoder_do_decoder_handler(void *arg)
{
    MP3_LOG("mp3_decoder_do_decoder_handler: mp3_decoder_state = %s.\r\n", mp3_decoder_state_str[mp3_decoder_state]);
    uint32_t available_data = 0;

    switch(mp3_decoder_state) {
        case MP3_DECODER_IDLE:
            mp3_decoder_state = MP3_DECODER_BUFFERING;
        case MP3_DECODER_BUFFERING:
            if(MP3_DECODER_BUFFERING_LIMIT <= mp3_decoder_total_input) {
                mp3_decoder_engine_init();
                mp3_decoder_state = MP3_DECODER_INITIATING;
            }
            else {
                break;
            }
        case MP3_DECODER_INITIATING:
            if(mp3_decoder_engine_start() == 1) {
                /* once initialization is done, start decode the first frame directly */
                mp3_decoder_state = MP3_DECODER_DECODING;
            }
            else {
            	/* try to restart initialization if start failed */
                if(mp3_decoder_prep_for_next == false) {
            	    struct task_msg_t *msg;
            	    msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
            	    task_msg_insert(msg);
                }
                break;
            }
        case MP3_DECODER_DECODING:
            mp3_decoder_engine_decode();
            break;
        case MP3_DECODER_PCM_TRANSMITTING:
        default:
            break;
    }
}

void mp3_decoder_init(void *arg)
{
    uint8_t channel;

    channel = ipc_alloc_channel(0);
    if(channel != 0xff) {
        ui_init_done = 0;
        ui_exec_done = 0;

        mp3_decoder_buffer = mp3_mem_alloc(MP3_DECODER_BUFFER_SIZE);

        mp3_decoder_buffer_input_index = 0;
        mp3_decoder_buffer_output_index = 0;

        mp3_decoder_dynamic_consumed = 0;
        mp3_decoder_total_input = 0;
        mp3_decoder_state = MP3_DECODER_IDLE;

        uint32_t apply_unit;
        if(MP3_DECODER_APPLY_LIMIT < MP3_DECODER_BUFFER_SIZE) {
            apply_unit = MP3_DECODER_APPLY_LIMIT/256 - 1;
        }
        else {
            apply_unit = MP3_DECODER_BUFFER_SIZE/256 - 1;
        }
        ipc_insert_msg(channel, IPC_MSG_RAW_BUFFER_SPACE, apply_unit, ipc_free_channel);

        mp3_decoder_all_request_data_received = false;
        mp3_decoder_request_data_size = apply_unit*256;

        mp3_decoder_prep_for_next = false;

        resample_used = false;
    }
}

void mp3_decoder_destroy(void)
{
    uint32_t i;

    for(i=0; i<g_w_malloc_count; i++) {
        mp3_mem_free(g_pv_arr_alloc_memory[i]);
    }
    g_w_malloc_count = 0;

    mp3_mem_free(mp3_decoder_buffer);
    mp3_decoder_buffer = NULL;

    mp3_decoder_dynamic_consumed = 0;
    mp3_decoder_total_input = 0;
    mp3_decoder_state = MP3_DECODER_IDLE;
    mp3_decoder_all_request_data_received = false;

    if(resample_used) {
#if USE_DOWNSAMPLE == 1
        downsample_destroy();
#else   // USE_DOWNSAMPLE == 1
        upsample_destroy();
#endif  // USE_DOWNSAMPLE == 1
    }
}

void mp3_decoder_prepare_for_next(void)
{
    mp3_decoder_prep_for_next = true;
}

void mp3_decoder_recv_frame(pWORD8 buffer, UWORD32 length)
{
    struct task_msg_t *msg;
    int left_space = MP3_DECODER_BUFFER_SIZE - mp3_decoder_buffer_input_index;

//    uart_putc_noint_no_wait('R');

    if(0) {
        pUWORD8 ptr = (pUWORD8)buffer;
        for(uint32_t i=0; i<length; i++) {
            printf("%02x", ptr[i]);
        }
        printf("\r\n");
    }

    /* check whether all request data have been received */
    if(mp3_decoder_request_data_size >= length) {
    	mp3_decoder_request_data_size -= length;
    	if(mp3_decoder_request_data_size == 0) {
    		mp3_decoder_all_request_data_received = true;
    	}
    }
    else {
    	mp3_decoder_request_data_size = 0;
    	mp3_decoder_all_request_data_received = true;
    }

    mp3_decoder_total_input += length;

    /* copy received data to local buffer */
    if(left_space <= length) {
        memcpy(mp3_decoder_buffer+mp3_decoder_buffer_input_index, buffer, left_space);
        length -= left_space;
        buffer += left_space;
        mp3_decoder_buffer_input_index = 0;
        if(length) {
            memcpy(mp3_decoder_buffer, buffer, length);
            mp3_decoder_buffer_input_index = length;
        }
    }
    else {
        memcpy(mp3_decoder_buffer+mp3_decoder_buffer_input_index, buffer, length);
        mp3_decoder_buffer_input_index += length;
    }

    MP3_LOG("mp3_decoder_recv_frame: mp3_decoder_total_input = %d, mp3_decoder_buffer_input_index = %d.\r\n", mp3_decoder_total_input, mp3_decoder_buffer_input_index);

    if((mp3_decoder_state == MP3_DECODER_IDLE) && (MP3_DECODER_BUFFERING_LIMIT <= mp3_decoder_total_input)) {
    	mp3_decoder_state = MP3_DECODER_BUFFERING;
        msg = task_msg_alloc(MP3_DECODER_DO_DECODE, 0);
        task_msg_insert(msg);
    }
}
