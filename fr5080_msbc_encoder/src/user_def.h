/*
 * user_def.h
 *
 *  Created on: 2018-9-20
 *      Author: owen
 */

#ifndef _USER_DEF
#define _USER_DEF

#include "tasks.h"

enum ipc_user_sub_msg_type_t {
    IPC_SUB_MSG_NEED_MORE_SBC,  // MCU notice DSP to send more sbc frame
    IPC_SUB_MSG_DECODER_START,  // MCU notice DSP to initiate MP3, AAC, etc. decoder
    IPC_SUB_MSG_REINIT_DECODER, // MCU notice DSP to reinitiate decoder engine
    IPC_SUB_MSG_NREC_START,
    IPC_SUB_MSG_NREC_STOP,
    IPC_SUB_MSG_FLASH_COPY_ACK,
    IPC_SUM_MSG_DSP_USER_CODE_READY,
    IPC_SUB_MSG_DECODER_STOP,   // MCU notice DSP to stop adecoder
    IPC_SUB_MSG_DECODER_PREP_NEXT,  // MCU notice DSP to prepare for next song
    IPC_SUB_MSG_DECODER_PREP_READY, // DSP notice MCU ready to play next song
    IPC_SUB_MSG_DECODER_START_LOCAL,// MCU notice DSP to initiate MP3, AAC, etc. decoder for native playback
    IPC_SUB_MSG_NEED_MORE_MSBC,  // MCU notice DSP to send more msbc frame
};

enum ipc_user_msg_type_t {
    /*IPC_MSG_LOAD_CODE = 0,
    IPC_MSG_LOAD_CODE_DONE = 1,
    IPC_MSG_EXEC_USER_CODE = 2,
    IPC_MSG_DSP_READY = 10,*/
    IPC_MSG_RAW_FRAME = 3,          // MCU send new raw frame to DSP
    IPC_MSG_DECODED_PCM_FRAME = 4,  // DSP send decoded pcm data to MCU
    IPC_MSG_ENCODED_SBC_FRAME = 5,  // DSP send encoded sbc frame to MCU
    IPC_MSG_WITHOUT_PAYLOAD = 6,    // some command without payload, use length segment in ipc-msg to indicate sub message
    IPC_MSG_RAW_BUFFER_SPACE = 7,   // used by DSP to tell MCU how much buffer space left to save raw data, return_value = actual_length / 256
    IPC_MSG_FLASH_OPERATION = 8,
    IPC_MSG_SET_SBC_CODEC_PARAM = 9, //MCU send codec parameters to DSP(bitpool,sample rate...)
    IPC_MSG_ENCODED_MSBC_FRAME = 11,  // DSP send encoded sbc frame to MCU
};

struct sbc_info_t {
    uint8_t bit_pool;
    uint8_t sample_rate;
    uint8_t channel_mode;
    uint8_t alloc_method;
    uint8_t blocks;
    uint8_t sub_bands;
    uint8_t channels;
};
#endif /* _USER_DEF */
