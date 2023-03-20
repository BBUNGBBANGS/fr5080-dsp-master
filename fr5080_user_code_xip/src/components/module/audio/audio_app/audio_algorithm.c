#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <xtensa/xtruntime.h>

#include "tasks.h"
#include "co_mem.h"

#include "audio_algorithm.h"

#include "drc.h"
#include "cvsd_plc.h"
#include "aec_core.h"
#include "noise_suppression.h"
#include "gain_control.h"
#include "echo_cancellation.h"

#include "plf.h"
#include "ipc.h"
#include "uart.h"
#include "user_def.h"
#include "hal_gpio.h"

#define AUDIO_ALGO_LOG				//printf
#define AUDIO_ALGO_SPK_LOG          0
#define AUDIO_ALGO_ESCO_OUT_LOG     0
#define AUDIO_ALGO_MIC_LOG          0
#define AUDIO_ALGO_ESCO_IN_LOG      0
#define AUDIO_ALGO_PTR_LOG          0

#define AUDIO_ALGO_DBG_WITH_IO      0

#define REG_PL_WR(addr, data)       *(volatile uint32_t *)(addr) = (data)
#define REG_PL_RD(addr)             *(volatile uint32_t *)(addr)

#define ENABLE_PLC          1
#define ENABLE_AEC          1
#define ENABLE_NS           1
#define ENABLE_AGC          1

char char2ascii[] = "0123456789abcdef";

struct audio_algorithm_t *audio_algorithm_env = NULL;

#define AEC_OUT_BUFFER_FRAME_COUNT  6
#define AUDIO_ALG_FRAME_SIZE        30
#define AUDIO_ALG_AEC_SIZE          160
#define AUDIO_ALG_AEC_OFFSET        (AUDIO_ALG_AEC_SIZE + 30)   //1A 1B 之间的距离
#define PLC_BUFFER_MAX              900     //30X
#define LOCAL_IN_BUFFER_MAX         900     //30X
#define AEC_OUT_BUFFER_MAX          (AUDIO_ALG_AEC_SIZE*AEC_OUT_BUFFER_FRAME_COUNT)

#define IPC_DMA_RX_INT_OCCURED      0x01
#define IPC_DMA_TX_INT_OCCURED      0x02

struct audio_algorithm_t {
    int16_t plc_buffer[PLC_BUFFER_MAX];
    uint32_t plc_in_data_state;         //每一个比特代表一帧数据的状态,因此PLC_BUFFER_MAX不要超过960
    int16_t plc_buffer_insert_offset;  //远端数据读入指针1D
    int16_t plc_buffer_write_offset;   //PLC 输入数据的指针1C
    int16_t plc_buffer_read_offset;    //写入到codec的指针1B
    int16_t plc_buffer_aec_offset;     //用于AEC算法的指针1A
    int16_t plc_buffer_max;

    int16_t local_in_buffer[LOCAL_IN_BUFFER_MAX];
    int16_t local_in_buffer_write_offset;  //2B
    int16_t local_in_buffer_read_offset;   //2A
    int16_t local_in_buffer_max;

    int16_t aec_out_buffer[AEC_OUT_BUFFER_MAX];
    int16_t aec_out_buffer_write_offset;   //3B
    int16_t aec_out_buffer_read_offset;    //3A
    int16_t aec_out_buffer_max;

    cvsd_plc_state_t plc_state;
#if ENABLE_AEC
    aec_t *aec_core;
#endif
#if ENABLE_AGC
    void *agc_handle;
#endif
#if ENABLE_NS
    NsHandle *ns_handle;
#endif

    int16_t aec_remote_data[AUDIO_ALG_AEC_SIZE];
    int16_t aec_local_data[AUDIO_ALG_AEC_SIZE];
    uint8_t enable;
    uint8_t ipc_dma_int_occur_status;
    uint8_t launch_algo_msg_count;

    uint8_t rx_toggle;
    uint8_t tx_toggle;
};

#define CO_BIT(x)               (1<<x)
#define AUDIO_ALG_PLC_ENABLE    CO_BIT(3)
#define AUDIO_ALG_AEC_ENABLE    CO_BIT(0)
#define AUDIO_ALG_NR_ENABLE     CO_BIT(1)
#define AUDIO_ALG_AGC_ENABLE    CO_BIT(2)
uint8_t audio_alg_sel = (AUDIO_ALG_PLC_ENABLE|AUDIO_ALG_AEC_ENABLE|AUDIO_ALG_NR_ENABLE|AUDIO_ALG_AGC_ENABLE);

#define AUDIO_ALGO_PRINT_SEL_ESCO_OUT   CO_BIT(0)
#define AUDIO_ALGO_PRINT_SEL_ESCO_IN    CO_BIT(1)
#define AUDIO_ALGO_PRINT_SEL_MIC        CO_BIT(2)
#define AUDIO_ALGO_PRINT_SEL_SPK        CO_BIT(3)
#define AUDIO_ALGO_PRINT_SEL_PTR        CO_BIT(4)
uint8_t audio_alg_print_sel = 0;

/*如果大于0,表示回音通路短,需要用旧一些的local_voice;
    如果小于0,表示回音通路长,需要用旧一些的remote_voice; */
int16_t aec_shift = -240;

int16_t mic_data[60];
int16_t esco_in_data[60];

void audio_algorithm_init(void)
{
    //uint32_t total_buffer_len;
    struct audio_algorithm_t *audio_algorithm_env_tmp;
#if ENABLE_AGC
    WebRtcAgc_config_t agcConfig;
#endif

    audio_algorithm_env_tmp = (struct audio_algorithm_t *)pvPortMalloc(sizeof(struct audio_algorithm_t));
    memset((void *)audio_algorithm_env_tmp, 0, sizeof(struct audio_algorithm_t));

#if ENABLE_AEC
    WebRtcAec_CreateAec(&audio_algorithm_env_tmp->aec_core);
    WebRtcAec_InitAec(audio_algorithm_env_tmp->aec_core, 8000);
#endif
#if ENABLE_AGC
    WebRtcAgc_Create(&audio_algorithm_env_tmp->agc_handle);
#endif
#if ENABLE_NS
    WebRtcNs_Create(&audio_algorithm_env_tmp->ns_handle);
#endif

#if ENABLE_AEC
    //-----------AEC-----------------------
    audio_algorithm_env_tmp->aec_core->targetSupp = -18.4f;
    audio_algorithm_env_tmp->aec_core->minOverDrive = 5.0f;
#endif

#if ENABLE_AGC
    //-----------AGC-----------------------
    WebRtcAgc_Init(audio_algorithm_env_tmp->agc_handle, 0, 255, kAgcModeFixedDigital, 8000);
    agcConfig.compressionGaindB = 20;
    agcConfig.limiterEnable     = 1;
    agcConfig.targetLevelDbfs   = 3;
    WebRtcAgc_set_config(audio_algorithm_env_tmp->agc_handle, agcConfig);
#endif

#if ENABLE_NS
    //-----------NS--------------------
    WebRtcNs_Init(audio_algorithm_env_tmp->ns_handle, 8000);
    WebRtcNs_set_policy(audio_algorithm_env_tmp->ns_handle, 2);
#endif

    //-----------PLC--------------------
    cvsd_plc_init(&audio_algorithm_env_tmp->plc_state);
    
    //-----------DRC--------------------
    struct drc_param param;
    param.initial_gain = 0;
    param.target_threshold = -12;
    drc_init(NULL);

    audio_algorithm_env_tmp->plc_buffer_max = PLC_BUFFER_MAX;
    audio_algorithm_env_tmp->local_in_buffer_max = LOCAL_IN_BUFFER_MAX;
    audio_algorithm_env_tmp->aec_out_buffer_max = AEC_OUT_BUFFER_MAX;
    if(aec_shift > 0) {
        audio_algorithm_env_tmp->local_in_buffer_read_offset = LOCAL_IN_BUFFER_MAX - aec_shift;
    }
    else {
        audio_algorithm_env_tmp->local_in_buffer_read_offset = 0;
    }

    audio_algorithm_env_tmp->plc_buffer_read_offset = PLC_BUFFER_MAX - 8*AUDIO_ALG_FRAME_SIZE;
    if(aec_shift > 0) {
        audio_algorithm_env_tmp->plc_buffer_aec_offset = audio_algorithm_env_tmp->plc_buffer_read_offset;
    }
    else {
        audio_algorithm_env_tmp->plc_buffer_aec_offset = audio_algorithm_env_tmp->plc_buffer_read_offset + aec_shift;
    }

    audio_algorithm_env_tmp->aec_out_buffer_read_offset = (AEC_OUT_BUFFER_FRAME_COUNT - 3) * AUDIO_ALG_AEC_SIZE;

    audio_algorithm_env_tmp->enable = true;

    audio_algorithm_env = audio_algorithm_env_tmp;

    AUDIO_ALGO_LOG("audio_algorithm_init: %08x, %d\r\n", audio_algorithm_env, sizeof(struct audio_algorithm_t));
}

void audio_algorithm_release(void *msg)
{
    struct audio_algorithm_t *audio_algorithm_env_tmp = audio_algorithm_env;

    AUDIO_ALGO_LOG("audio_algorithm_release\r\n");

    _xtos_interrupt_disable(XCHAL_IPC_DMA_RX_INTERRUPT);
    _xtos_interrupt_disable(XCHAL_IPC_DMA_TX_INTERRUPT);

    ipc_dma_rx_int_clear();
    ipc_dma_tx_int_clear();

    if(audio_algorithm_env == NULL) {
        return;
    }

    audio_algorithm_env = NULL;
#if ENABLE_AEC
    WebRtcAec_FreeAec(audio_algorithm_env_tmp->aec_core);
#endif
#if ENABLE_AGC
    WebRtcAgc_Free(audio_algorithm_env_tmp->agc_handle);
#endif
#if ENABLE_NS
    WebRtcNs_Free(audio_algorithm_env_tmp->ns_handle);
#endif

    drc_destroy();

    vPortFree((void *)audio_algorithm_env_tmp);
}

__attribute__((section("iram_section"))) int16_t *audio_algorithm_get_plc_insert_buffer(uint8_t esco_data_mute)
{
    int16_t *buffer;
    uint8_t offset;

    AUDIO_ALGO_LOG("audio_algorithm_get_plc_insert_buffer: %d %d.\r\n", audio_algorithm_env->plc_buffer_insert_offset, esco_data_mute);

    buffer = &audio_algorithm_env->plc_buffer[audio_algorithm_env->plc_buffer_insert_offset];
    offset = audio_algorithm_env->plc_buffer_insert_offset / AUDIO_ALG_FRAME_SIZE;
    if(esco_data_mute) {
        audio_algorithm_env->plc_in_data_state |= (1<<offset);
    }
    else {
        audio_algorithm_env->plc_in_data_state &= (~(1<<offset));
    }
    audio_algorithm_env->plc_buffer_insert_offset += AUDIO_ALG_FRAME_SIZE;
    if(audio_algorithm_env->plc_buffer_insert_offset >= audio_algorithm_env->plc_buffer_max) {
        audio_algorithm_env->plc_buffer_insert_offset = 0;
    }

    return buffer;
}

int16_t *audio_algorithm_get_plc_out_buffer(void)
{
    int16_t *buffer;

    AUDIO_ALGO_LOG("audio_algorithm_get_plc_out_buffer: %d\r\n", audio_algorithm_env->plc_buffer_read_offset);

    buffer = &audio_algorithm_env->plc_buffer[audio_algorithm_env->plc_buffer_read_offset];
    audio_algorithm_env->plc_buffer_read_offset += AUDIO_ALG_FRAME_SIZE;
    if(audio_algorithm_env->plc_buffer_read_offset >= audio_algorithm_env->plc_buffer_max) {
        audio_algorithm_env->plc_buffer_read_offset = 0;
    }

    return buffer;
}

__attribute__((section("iram_section"))) int16_t *audio_algorithm_get_local_in_write_buffer(void)
{
    int16_t *buffer;

    AUDIO_ALGO_LOG("audio_algorithm_get_local_in_write_buffer: %d %d\r\n", audio_algorithm_env->local_in_buffer_write_offset, audio_algorithm_env->local_in_buffer_max);

    buffer = &audio_algorithm_env->local_in_buffer[audio_algorithm_env->local_in_buffer_write_offset];
    audio_algorithm_env->local_in_buffer_write_offset += AUDIO_ALG_FRAME_SIZE;
    if(audio_algorithm_env->local_in_buffer_write_offset >= audio_algorithm_env->local_in_buffer_max) {
        audio_algorithm_env->local_in_buffer_write_offset = 0;
    }

    return buffer;
}

void audio_alg_launch(bool execute)
{
    int16_t *out;
    int16_t store_size, head_len, tail_len, begin;
    uint8_t offset;
    uint32_t out_mic_level;
    uint8_t saturationWarning;
    int16_t out_frame[AUDIO_ALG_AEC_SIZE], out_frame_middle[AUDIO_ALG_AEC_SIZE];

    if(audio_algorithm_env == NULL) {
        return;
    }

    cvsd_plc_state_t *plc_state = &audio_algorithm_env->plc_state;

    //GLOBAL_INT_DISABLE();
    //uart_putc_noint('<');

#if AUDIO_ALGO_DBG_WITH_IO
    gpio_set(0);
    //*(volatile uint32_t *)GPIO_DIR = (AUDIO_ALG_AEC_ENABLE|AUDIO_ALG_NR_ENABLE|AUDIO_ALG_AGC_ENABLE);
    //audio_alg_sel = *(volatile uint32_t *)GPIO_DATA;
    //audio_alg_sel = AUDIO_ALG_AEC_ENABLE|AUDIO_ALG_NR_ENABLE;
    //audio_alg_sel = AUDIO_ALG_AEC_ENABLE;
    //audio_alg_sel = 0;
#endif

    out = &audio_algorithm_env->plc_buffer[audio_algorithm_env->plc_buffer_write_offset];
    offset = audio_algorithm_env->plc_buffer_write_offset / AUDIO_ALG_FRAME_SIZE;
    if(audio_algorithm_env->plc_buffer_write_offset >= audio_algorithm_env->plc_buffer_read_offset) {
        store_size = audio_algorithm_env->plc_buffer_write_offset - audio_algorithm_env->plc_buffer_read_offset;
    }
    else {
        store_size = audio_algorithm_env->plc_buffer_write_offset + audio_algorithm_env->plc_buffer_max - audio_algorithm_env->plc_buffer_read_offset;
    }
    audio_algorithm_env->plc_buffer_write_offset += AUDIO_ALG_FRAME_SIZE;
    if(audio_algorithm_env->plc_buffer_write_offset >= audio_algorithm_env->plc_buffer_max) {
        audio_algorithm_env->plc_buffer_write_offset = 0;
    }

#if ENABLE_PLC
    {
        if ((store_size >= 2*AUDIO_ALG_FRAME_SIZE)  //保证read不超过write, 也就是说剩余有效数据播放时间不应小于做一次PLC的时间
            &&(audio_algorithm_env->plc_in_data_state & (1<<offset))){
        	AUDIO_ALGO_LOG("audio_alg_launch: plc bad.\r\n");
            if (plc_state->good_frames_nr > CVSD_LHIST/CVSD_FS){
                cvsd_plc_bad_frame(plc_state, out);

                plc_state->bad_frames_nr++;
            }
            else {
                memset(out, 0, CVSD_FS);
            }
        }
        else {
        	AUDIO_ALGO_LOG("audio_alg_launch: plc good.\r\n");

            cvsd_plc_good_frame(plc_state, out);
            plc_state->good_frames_nr++;   
        }
    }
#endif

    drc_run(out, 30);

    if(aec_shift < 0) {
        store_size = (audio_algorithm_env->local_in_buffer_write_offset + audio_algorithm_env->local_in_buffer_max - audio_algorithm_env->local_in_buffer_read_offset) % audio_algorithm_env->local_in_buffer_max;
    }
    else {
        store_size = (audio_algorithm_env->plc_buffer_read_offset + audio_algorithm_env->plc_buffer_max - audio_algorithm_env->plc_buffer_aec_offset) % audio_algorithm_env->plc_buffer_max;
    }
    if(store_size >= AUDIO_ALG_AEC_OFFSET) {
        begin = audio_algorithm_env->plc_buffer_aec_offset;
        tail_len = audio_algorithm_env->plc_buffer_max - begin;
        if(tail_len >= AUDIO_ALG_AEC_SIZE) {
            memcpy(&audio_algorithm_env->aec_remote_data[0], &audio_algorithm_env->plc_buffer[begin], AUDIO_ALG_AEC_SIZE*sizeof(int16_t));

            audio_algorithm_env->plc_buffer_aec_offset += AUDIO_ALG_AEC_SIZE;
            if(audio_algorithm_env->plc_buffer_aec_offset >= audio_algorithm_env->plc_buffer_max) {
                audio_algorithm_env->plc_buffer_aec_offset = 0;
            }
        }
        else {
            memcpy(&audio_algorithm_env->aec_remote_data[0], &audio_algorithm_env->plc_buffer[begin], tail_len*sizeof(int16_t));
            head_len = AUDIO_ALG_AEC_SIZE - tail_len;
            memcpy(&audio_algorithm_env->aec_remote_data[tail_len], &audio_algorithm_env->plc_buffer[0], head_len*sizeof(int16_t));
            audio_algorithm_env->plc_buffer_aec_offset = head_len;
        }

        begin = audio_algorithm_env->local_in_buffer_read_offset;
        tail_len = audio_algorithm_env->local_in_buffer_max - begin;
        if(tail_len >= AUDIO_ALG_AEC_SIZE) {
            memcpy(&audio_algorithm_env->aec_local_data[0], &audio_algorithm_env->local_in_buffer[begin], AUDIO_ALG_AEC_SIZE*sizeof(int16_t));

            audio_algorithm_env->local_in_buffer_read_offset += AUDIO_ALG_AEC_SIZE;
            if(audio_algorithm_env->local_in_buffer_read_offset >= audio_algorithm_env->local_in_buffer_max) {
                audio_algorithm_env->local_in_buffer_read_offset = 0;
            }
        }
        else {
            memcpy(&audio_algorithm_env->aec_local_data[0], &audio_algorithm_env->local_in_buffer[begin], tail_len*sizeof(int16_t));
            head_len = AUDIO_ALG_AEC_SIZE - tail_len;
            memcpy(&audio_algorithm_env->aec_local_data[tail_len], &audio_algorithm_env->local_in_buffer[0], head_len*sizeof(int16_t));
            
            audio_algorithm_env->local_in_buffer_read_offset = head_len;
        }

#if ENABLE_NS
        if(execute && ((audio_alg_sel & AUDIO_ALG_NR_ENABLE)!=0)) {
            WebRtcNs_Process(audio_algorithm_env->ns_handle,
                                (short *)&audio_algorithm_env->aec_local_data[0],
                                0,
                                out_frame,
                                0);
            WebRtcNs_Process(audio_algorithm_env->ns_handle,
                                (short *)&audio_algorithm_env->aec_local_data[AUDIO_ALG_AEC_SIZE>>1],
                                0,
                                &out_frame[AUDIO_ALG_AEC_SIZE>>1],
                                0);
        }
        else {
            memcpy((void *)&out_frame[0], (void *)&audio_algorithm_env->aec_local_data[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
        }
#else
        memcpy((void *)&out_frame[0], (void *)&audio_algorithm_env->aec_local_data[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
#endif

#if ENABLE_AGC
        if(execute && ((audio_alg_sel & AUDIO_ALG_AGC_ENABLE)!=0)) {
            WebRtcAgc_Process(audio_algorithm_env->agc_handle,
                                out_frame, 0,
                                AUDIO_ALG_AEC_SIZE,
                                out_frame_middle, 0, 0,
                                &out_mic_level, 0,
                                &saturationWarning);
        }
        else {
            memcpy((void *)&out_frame_middle[0], (void *)&out_frame[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
        }
#else
        memcpy((void *)&out_frame_middle[0], (void *)&out_frame[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
#endif

#if ENABLE_AEC
        if(execute && ((audio_alg_sel & AUDIO_ALG_AEC_ENABLE)!=0)) {
            WebRtcAec_ProcessFrame(audio_algorithm_env->aec_core,
                                    &audio_algorithm_env->aec_remote_data[0],
                                    out_frame_middle, 0,
                                    &audio_algorithm_env->aec_out_buffer[audio_algorithm_env->aec_out_buffer_write_offset],
                                    0, 0);
        }
        else {
            memcpy((void *)&audio_algorithm_env->aec_out_buffer[audio_algorithm_env->aec_out_buffer_write_offset],
                                        (void *)&out_frame_middle[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
        }
#else
        memcpy((void *)&audio_algorithm_env->aec_out_buffer[audio_algorithm_env->aec_out_buffer_write_offset], 
                            (void *)&out_frame_middle[0], AUDIO_ALG_AEC_SIZE * sizeof(int16_t));
#endif

#if 0
       //if(pskeys.bt_options & PSKEY_ENABLE_AUDIO_PRINT_D) {
            for(uint32_t i=0; i<AUDIO_ALG_AEC_SIZE; i++) {
                int16_t data = audio_algorithm_env->aec_out_buffer[audio_algorithm_env->aec_out_buffer_write_offset+i];
                fputc(char2ascii[(data>>12)&0x0f], 0);
                fputc(char2ascii[(data>>8)&0x0f], 0);
                fputc(char2ascii[(data>>4)&0x0f], 0);
                fputc(char2ascii[(data)&0x0f], 0);
#if 0
                data = audio_algorithm_env->aec_remote_data[i];
                fputc(char2ascii[(data>>12)&0x0f], 0);
                fputc(char2ascii[(data>>8)&0x0f], 0);
                fputc(char2ascii[(data>>4)&0x0f], 0);
                fputc(char2ascii[(data)&0x0f], 0);
#endif
            }
            fputc('\n', 0);
        //}
#endif

        audio_algorithm_env->aec_out_buffer_write_offset += AUDIO_ALG_AEC_SIZE;
        if(audio_algorithm_env->aec_out_buffer_write_offset >= audio_algorithm_env->aec_out_buffer_max) {
            audio_algorithm_env->aec_out_buffer_write_offset = 0;
        }
    }

#if AUDIO_ALGO_DBG_WITH_IO
    gpio_clear(0);
#endif
    //uart_putc_noint('>');
    //GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) void audio_algorithm_recv_data(int16_t *mic, int16_t *esco_in, uint8_t esco_data_mute)
{
    int16_t *tmp, *tmp1, value;
    uint32_t i;
    
    tmp = audio_algorithm_get_local_in_write_buffer();
    tmp1 = audio_algorithm_get_plc_insert_buffer(esco_data_mute);

    if(esco_data_mute) {
        for(i=0; i<AUDIO_ALG_FRAME_SIZE; i++) {
            #if 1
            value = 0x0000;
            *tmp1++ = value;
#if AUDIO_ALGO_ESCO_IN_LOG
            if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_ESCO_IN) {
                uart_putc_noint(char2ascii[(value>>12)&0x0f]);
                uart_putc_noint(char2ascii[(value>>8)&0x0f]);
                uart_putc_noint(char2ascii[(value>>4)&0x0f]);
                uart_putc_noint(char2ascii[(value)&0x0f]);
            }
#endif
            value = *mic++;
            *tmp++ = value;
#if AUDIO_ALGO_MIC_LOG
            if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_MIC) {
                uart_putc_noint(char2ascii[(value>>12)&0x0f]);
                uart_putc_noint(char2ascii[(value>>8)&0x0f]);
                uart_putc_noint(char2ascii[(value>>4)&0x0f]);
                uart_putc_noint(char2ascii[(value)&0x0f]);
            }
#endif
            #else
            *tmp1++ = 0x0000;
            *tmp++ = *mic++;
            #endif
        }
    }
    else {
        for(i=0; i<AUDIO_ALG_FRAME_SIZE; i++) {
            #if 1
            value = *esco_in++;
            *tmp1++ = value;
#if AUDIO_ALGO_ESCO_IN_LOG
            if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_ESCO_IN) {
                uart_putc_noint(char2ascii[(value>>12)&0x0f]);
                uart_putc_noint(char2ascii[(value>>8)&0x0f]);
                uart_putc_noint(char2ascii[(value>>4)&0x0f]);
                uart_putc_noint(char2ascii[(value)&0x0f]);
            }
#endif
            value = *mic++;
            *tmp++ = value;
#if AUDIO_ALGO_MIC_LOG
            if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_MIC) {
                uart_putc_noint(char2ascii[(value>>12)&0x0f]);
                uart_putc_noint(char2ascii[(value>>8)&0x0f]);
                uart_putc_noint(char2ascii[(value>>4)&0x0f]);
                uart_putc_noint(char2ascii[(value)&0x0f]);
            }
#endif
            #else
            *tmp1++ = *esco_in++;
            *tmp++ = *mic++;
            #endif
        }
    }
#if AUDIO_ALGO_MIC_LOG || AUDIO_ALGO_ESCO_IN_LOG
    if(audio_alg_print_sel & (AUDIO_ALGO_PRINT_SEL_ESCO_IN|AUDIO_ALGO_PRINT_SEL_MIC)) {
        uart_putc_noint('\r');
        uart_putc_noint('\n');
    }
#endif
}

__attribute__((section("iram_section"))) void audio_algorithm_send_data(int16_t *spk, int16_t *spk_i2s, int16_t *esco_out)
{
    int16_t *tmp, value;
    uint32_t i, j;
    
    tmp = audio_algorithm_get_plc_out_buffer();
    for(i=0; i<AUDIO_ALG_FRAME_SIZE; i++) {
        #if 1
        value = *tmp++;
        *spk++ = value;
        *spk_i2s++ = value;
#if AUDIO_ALGO_SPK_LOG
        if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_SPK) {
            uart_putc_noint(char2ascii[(value>>12)&0x0f]);
            uart_putc_noint(char2ascii[(value>>8)&0x0f]);
            uart_putc_noint(char2ascii[(value>>4)&0x0f]);
            uart_putc_noint(char2ascii[(value)&0x0f]);
        }
#endif
        #else
        *spk++ = *tmp;
        *spk_i2s++ = *tmp++;
        #endif
    }
#if AUDIO_ALGO_SPK_LOG
    if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_SPK) {
        uart_putc_noint('\r');
        uart_putc_noint('\n');
    }
#endif

    i = audio_algorithm_env->aec_out_buffer_max - audio_algorithm_env->aec_out_buffer_read_offset;
    tmp = &audio_algorithm_env->aec_out_buffer[audio_algorithm_env->aec_out_buffer_read_offset];
    if(i >= AUDIO_ALG_FRAME_SIZE) {
        i = AUDIO_ALG_FRAME_SIZE;
        audio_algorithm_env->aec_out_buffer_read_offset += AUDIO_ALG_FRAME_SIZE;
        if(audio_algorithm_env->aec_out_buffer_read_offset >= audio_algorithm_env->aec_out_buffer_max) {
            audio_algorithm_env->aec_out_buffer_read_offset = 0;
        }
        j = 0;
    }
    else {
        j = AUDIO_ALG_FRAME_SIZE - i;
        audio_algorithm_env->aec_out_buffer_read_offset = j;
    }
    for(; i>0; i--) {
        #if 1
        value = *tmp++;
        *esco_out++ = value;
#if AUDIO_ALGO_ESCO_OUT_LOG
        if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_ESCO_OUT) {
            uart_putc_noint(char2ascii[(value>>12)&0x0f]);
            uart_putc_noint(char2ascii[(value>>8)&0x0f]);
            uart_putc_noint(char2ascii[(value>>4)&0x0f]);
            uart_putc_noint(char2ascii[(value)&0x0f]);
        }
#endif
        #else
        *esco_out++ = *tmp++;
        #endif
    }
    tmp = &audio_algorithm_env->aec_out_buffer[0];
    for(; j>0; j--) {
        #if 1
        value = *tmp++;
        *esco_out++ = value;
#if AUDIO_ALGO_ESCO_OUT_LOG
        if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_ESCO_OUT) {
            uart_putc_noint(char2ascii[(value>>12)&0x0f]);
            uart_putc_noint(char2ascii[(value>>8)&0x0f]);
            uart_putc_noint(char2ascii[(value>>4)&0x0f]);
            uart_putc_noint(char2ascii[(value)&0x0f]);
        }
#endif
        #else
        *esco_out++ = *tmp++;
        #endif
    }
#if AUDIO_ALGO_ESCO_OUT_LOG
    if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_ESCO_OUT) {
        uart_putc_noint('\r');
        uart_putc_noint('\n');
    }
#endif
}

__attribute__((section("iram_section"))) static void audio_algorithm_ipc_rx(void)
{
    struct task_msg_t *msg;
    
    ipc_dma_rx_int_clear();

    if(audio_algorithm_env == NULL) {
        return;
    }

    if(audio_algorithm_env->ipc_dma_int_occur_status != 0) {
        audio_algorithm_env->ipc_dma_int_occur_status--;
        if(audio_algorithm_env->ipc_dma_int_occur_status > 0) {
            return;
        }
    }

    int16_t *mic, *esco_in, *pdm_in, *tmp, value, i;

    if(ipc_get_esco_bb2dsp_tog(0)) {
        esco_in = (int16_t *)IPC_ESCO0_READ_BUFFER0;
    }
    else {
        esco_in = (int16_t *)IPC_ESCO0_READ_BUFFER1;
    }

	#if 1
	//mic from codec adc
    if(ipc_get_codec_adc_tog()) {
        mic = (int16_t *)IPC_CODEC_READ_BUFFER0;
    }
    else {
        mic = (int16_t *)IPC_CODEC_READ_BUFFER1;
    }
	#else
	/*mic from external i2s
    if(ipc_get_i2s_adc_tog()) {
        mic = (int16_t *)IPC_I2S_READ_BUFFER0;
    }
    else {
        mic = (int16_t *)IPC_I2S_READ_BUFFER1;
    }*/
    if(ipc_get_pdm_tog(0)) {
		mic = (int16_t *)IPC_PDM0_L_READ_BUFFER0;
	}
	else {
		mic = (int16_t *)IPC_PDM0_L_READ_BUFFER1;
	}
	#endif
	
	
#if 0
    if(last_mic_toggle) {
        last_mic_toggle = 0;
    }
    else {
        last_mic_toggle = 1;
    }
    memcpy((void *)&mic_data[last_mic_toggle][0], (void *)mic, AUDIO_ALG_FRAME_SIZE*2*sizeof(int16_t));
#endif
    //printf("mic=%x\r\n",mic[0]);
    audio_algorithm_recv_data(mic, esco_in, ipc_dma_esco_state_get());
    audio_algorithm_recv_data(mic+AUDIO_ALG_FRAME_SIZE, esco_in+AUDIO_ALG_FRAME_SIZE, ipc_dma_esco_state_get());

#if 0
    msg = task_msg_alloc(AUDIO_IPC_DMA_RX, 0);
    task_msg_insert(msg);
#else
#if 1
    audio_algorithm_env->launch_algo_msg_count++;
    msg = task_msg_alloc(AUDIO_IPC_DMA_RX, 0);
    task_msg_insert_front(msg);
#else
    if(audio_algorithm_env->launch_algo_msg_count > 4) {
        audio_alg_launch(false);
        audio_alg_launch(false);
    }
    else {
        audio_algorithm_env->launch_algo_msg_count++;
        msg = task_msg_alloc(AUDIO_IPC_DMA_RX, 0);
        task_msg_insert(msg);
    }
#endif
#endif
}

__attribute__((section("iram_section"))) static void audio_algorithm_ipc_tx(void)
{
    struct task_msg_t *msg;
    
    ipc_dma_tx_int_clear();
    
    if(audio_algorithm_env == NULL) {
        return;
    }

    if(audio_algorithm_env->ipc_dma_int_occur_status != 0) {
        return;
    }

#if AUDIO_ALGO_PTR_LOG
    if(audio_alg_print_sel & AUDIO_ALGO_PRINT_SEL_PTR) {
        uint16_t value;
        value = audio_algorithm_env->plc_buffer_aec_offset;
        uart_putc_noint('1');
        uart_putc_noint('A');
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('1');
        uart_putc_noint('B');
        value = audio_algorithm_env->plc_buffer_read_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('1');
        uart_putc_noint('C');
        value = audio_algorithm_env->plc_buffer_write_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('1');
        uart_putc_noint('D');
        value = audio_algorithm_env->plc_buffer_insert_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('2');
        uart_putc_noint('A');
        value = audio_algorithm_env->local_in_buffer_read_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('2');
        uart_putc_noint('B');
        value = audio_algorithm_env->local_in_buffer_write_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('3');
        uart_putc_noint('A');
        value = audio_algorithm_env->aec_out_buffer_read_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        uart_putc_noint('3');
        uart_putc_noint('B');
        value = audio_algorithm_env->aec_out_buffer_write_offset;
        uart_putc_noint(char2ascii[value/1000]);
        value %= 1000;
        uart_putc_noint(char2ascii[value/100]);
        value %= 100;
        uart_putc_noint(char2ascii[value/10]);
        value %= 10;
        uart_putc_noint(char2ascii[value]);
        if((audio_alg_print_sel & (AUDIO_ALGO_PRINT_SEL_ESCO_OUT|AUDIO_ALGO_PRINT_SEL_SPK))==0) {
            uart_putc_noint('\r');
            uart_putc_noint('\n');
        }
    }
#endif

#if 0
    msg = task_msg_alloc(AUDIO_IPC_DMA_TX, 0);
    task_msg_insert(msg);
#else
    audio_algorithm_ipc_tx_handler(NULL);
#endif
}

void audio_algorithm_ipc_rx_handler(struct task_msg_t *msg)
{
    GLOBAL_INT_DISABLE();
    if(audio_algorithm_env->launch_algo_msg_count != 0) {
        audio_algorithm_env->launch_algo_msg_count--;
    }
    GLOBAL_INT_RESTORE();
    audio_alg_launch(true);
    audio_alg_launch(true);
}

void audio_algorithm_ipc_tx_handler(struct task_msg_t *msg)
{
	uint8_t i;

    int16_t *spk, *esco_out, *spk_i2s;

    if(ipc_get_esco_dsp2bb_tog(0) == 0) {
        esco_out = (int16_t *)IPC_ESCO0_WRITE_BUFFER1;
    }
    else {
        esco_out = (int16_t *)IPC_ESCO0_WRITE_BUFFER0;
    }
    
    if(ipc_get_codec_dac_tog() == 0) {
        spk = (int16_t *)IPC_CODEC_WRITE_BUFFER1;
    }
    else {
        spk = (int16_t *)IPC_CODEC_WRITE_BUFFER0;
    }
    
    if(ipc_get_i2s_dac_tog() == 0) {
    	spk_i2s = (int16_t *)IPC_I2S_WRITE_BUFFER1;
    }
    else {
    	spk_i2s = (int16_t *)IPC_I2S_WRITE_BUFFER0;
    }

    audio_algorithm_send_data(spk, spk_i2s, esco_out);
#if AUDIO_ALGO_PTR_LOG && AUDIO_ALGO_ESCO_OUT_LOG
    if((audio_alg_print_sel & (AUDIO_ALGO_PRINT_SEL_PTR|AUDIO_ALGO_PRINT_SEL_ESCO_OUT))
            == (AUDIO_ALGO_PRINT_SEL_PTR|AUDIO_ALGO_PRINT_SEL_ESCO_OUT)) {
        for(uint8_t i=0; i<48; i++) {
            uart_putc_noint(' ');
        }
    }
#endif
    audio_algorithm_send_data(spk+AUDIO_ALG_FRAME_SIZE, spk_i2s+AUDIO_ALG_FRAME_SIZE, esco_out+AUDIO_ALG_FRAME_SIZE);
}
#if 0
static const struct task_msg_handler_t audio_algorithm_msg_table[] =
{
    {TASK_ID_IPC_DMA_RX,        audio_algorithm_ipc_rx_handler},
    {TASK_ID_IPC_DMA_TX,        audio_algorithm_ipc_tx_handler},
};
#endif


void audio_algorithm_ready(void)
{
    uint8_t channel;
        
    channel = ipc_alloc_channel(0);
    ipc_insert_msg(channel, IPC_MSG_DSP_READY, 0, NULL);
}

void audio_init(void *msg)
{
     uart_puts_noint("audio init\r\n");

    if(audio_algorithm_env != NULL) {
        return;
    }

    memset((void *)IPC_DSP_WRITE_BUFFER0, 0, 8*120);

    audio_algorithm_init();
    audio_algorithm_env->ipc_dma_int_occur_status = 40;

    ipc_dma_init(audio_algorithm_ipc_rx, audio_algorithm_ipc_tx);
    //REG_PL_WR(0x50000028,0x100); // enable ipc diagport
    _xtos_interrupt_enable(XCHAL_IPC_DMA_RX_INTERRUPT);
    _xtos_interrupt_enable(XCHAL_IPC_DMA_TX_INTERRUPT);
    //GLOBAL_INT_START();

    //audio_algorithm_ready();
	//app_register_default_task_handler(audio_task_handler);
   
#if AUDIO_ALGO_DBG_WITH_IO
    *(volatile uint32_t *)GPIO_DIR &= 0xfe;
#endif
}

#if AUDIO_ALGO_DBG_WITH_IO
#if 0
void uart_receive_char(uint8_t c)
{
    switch(c & 0xc0) {
        case 0x80:
            uart_putc_noint(audio_alg_sel);
            break;
        case 0x40:
            audio_alg_print_sel = c & (AUDIO_ALGO_PRINT_SEL_ESCO_OUT
                                        | AUDIO_ALGO_PRINT_SEL_ESCO_IN
                                        | AUDIO_ALGO_PRINT_SEL_SPK
                                        | AUDIO_ALGO_PRINT_SEL_MIC
                                        | AUDIO_ALGO_PRINT_SEL_PTR);
            break;
        case 0xc0:
            test_delay = 50*(c&0x3f);
            break;
        case 0x00:
            audio_alg_sel = c & (AUDIO_ALG_PLC_ENABLE|AUDIO_ALG_AEC_ENABLE|AUDIO_ALG_NR_ENABLE|AUDIO_ALG_AGC_ENABLE);
            break;
    }
}
#endif
#endif
