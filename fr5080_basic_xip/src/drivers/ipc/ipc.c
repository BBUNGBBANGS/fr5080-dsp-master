#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <xtensa/tie/xt_interrupt.h>

#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>

#include "ipc.h"
#include "plf.h"

#include "co_list.h"

/*
 * channel from 0 to IPC_WITH_LOAD_CHANNELS should be used to
 * transmit message with length from 0 to 1024 bytes. channel
 * from IPC_WITH_LOAD_CHANNELS to IPC_WITHOUT_LOAD_CHANNELS can
 * only transmit command with none payload.
 */
#define IPC_CHANNELS                4
#define IPC_WITHOUT_LOAD_CHANNELS   2
#define IPC_WITH_LOAD_CHANNELS      2

struct ipc_ctrl_t {
    uint32_t msgin00_int_en:1;
    uint32_t msgin01_int_en:1;
    uint32_t msgin10_int_en:1;
    uint32_t msgin11_int_en:1;
    uint32_t msgout00_int_en:1;
    uint32_t msgout01_int_en:1;
    uint32_t msgout10_int_en:1;
    uint32_t msgout11_int_en:1;
    uint32_t msgin00_int_status:1;
    uint32_t msgin01_int_status:1;
    uint32_t msgin10_int_status:1;
    uint32_t msgin11_int_status:1;
    uint32_t msgout00_int_status:1;
    uint32_t msgout01_int_status:1;
    uint32_t msgout10_int_status:1;
    uint32_t msgout11_int_status:1;
    uint32_t reserved:16;
};

struct ipc_dma_ctrl_t {
    uint32_t mcu2dsp_int_en:1;
    uint32_t dsp2mcu_int_en:1;
    uint32_t mcu2dsp_int_status:1;
    uint32_t dsp2mcu_int_status:1;
    uint32_t reserved0:4;
    uint32_t mcu2dsp_int_clear:1;
    uint32_t dsp2mcu_int_clear:1;
    uint32_t reserved1:22;
};

struct ipc_tog_info_t {
    uint32_t i2s_adc_tog:1;
    uint32_t codec_adc_tog:1;
    uint32_t pdm0_tog:1;
    uint32_t pdm1_tog:1;
    uint32_t pdm2_tog:1;
    uint32_t pdm3_tog:1;
    uint32_t esco0_des_tog:1;   // bb2dsp
    uint32_t esco1_des_tog:1;
    uint32_t i2s_dac_tog:1;
    uint32_t codec_dac_tog:1;
    uint32_t esco0_src_tog:1;   //dsp2bb
    uint32_t esco1_src_tog:1;
    uint32_t reserved:20;
};

struct ipc_dma_eco_packet_state_t {
    uint32_t esco_pkt_sta:2;
    uint32_t tws_pkt_sta:2;
    uint32_t toggle:1;
    uint32_t reserved:27;
};

struct ipc_t {
    struct ipc_ctrl_t ctr;
    uint32_t reserved0;
    struct ipc_msg_t msg_in[4];
    struct ipc_msg_t msg_out[4];
    uint32_t reserved1[2];

    struct ipc_dma_ctrl_t dma_ctrl;
    struct ipc_tog_info_t tog_info;
    struct ipc_dma_eco_packet_state_t esco_pkt_sta;
};

struct ipc_env_t {
    uint8_t available_buffer;

    uint8_t ipc_i2s_mcu2dsp_tog;
    uint8_t ipc_codec_mcu2dsp_tog;
    uint8_t ipc_pdm0_l_mcu2dsp_tog;
    uint8_t ipc_pdm0_r_mcu2dsp_tog;
    uint8_t ipc_pdm1_l_mcu2dsp_tog;
    uint8_t ipc_pdm1_r_mcu2dsp_tog;
    uint8_t ipc_esco0_mcu2dsp_tog;
    uint8_t ipc_esco1_mcu2dsp_tog;
    uint8_t ipc_i2s_dsp2mcu_tog;
    uint8_t ipc_codec_dsp2mcu_tog;
    uint8_t ipc_esco0_dsp2mcu_tog;
    uint8_t ipc_esco1_dsp2mcu_tog;

    ipc_tx_callback tx_callback[IPC_CHANNELS];
    ipc_rx_callback rx_callback;
};

struct ipc_msg_elem_t {
    struct co_list_hdr hdr;

    ipc_tx_callback callback;
    bool with_payload;

    enum ipc_msg_type_t msg;
    union {
        uint16_t sub_msg;
        struct {
            uint16_t length;
            uint8_t *buffer;
        } payload;
    } p;
};

const uint32_t ipc_buffer_offset[2][IPC_WITH_LOAD_CHANNELS] =
{
    {IPC_DSP_READ_OPT_BUFFER0,  IPC_DSP_READ_OPT_BUFFER1},
    {IPC_DSP_WRITE_OPT_BUFFER0, IPC_DSP_WRITE_OPT_BUFFER1},
};

static struct co_list ipc_msg_list = {NULL, NULL};

volatile struct ipc_t *ipc = (volatile struct ipc_t *)IPC_BASE;
struct ipc_env_t ipc_env;

void ipc_isr(void);
void ipc_dma_rx_isr(void);
void ipc_dma_tx_isr(void);

/*********************************************************************
 * @fn      ipc_init
 *
 * @brief   used to init ipc controller.
 *
 * @param   ints_en     - which interrupt source should be enable.
 *          callback    - callback function for rx-message
 *			
 * @return  None.
 */
__attribute__((section("need_kept"))) void ipc_init(uint32_t ints_en, ipc_rx_callback callback)
{
    *(uint32_t *)&ipc->ctr = ints_en;

    memset((void *)&ipc_env, 0, sizeof(ipc_env));
    ipc_env.available_buffer = (1<<IPC_CHANNELS) - 1;
    ipc_env.rx_callback = callback;

    _xtos_set_interrupt_handler(XCHAL_IPC_INTERRUPT, ipc_isr);
}

/*********************************************************************
 * @fn      ipc_alloc_channel
 *
 * @brief   allocate a channel for following transmit.
 *
 * @param   length      - the length of message. If this value is 0,
 *                      - channel 2 and 3 are also available.
 *			
 * @return  allocated channel number, 0xFF will return when none channel
 *          is avaliable.
 */
__attribute__((section("need_kept"))) uint8_t ipc_alloc_channel(uint16_t length)
{
    uint8_t found = false;
    uint8_t i = IPC_CHANNELS;

    GLOBAL_INT_DISABLE();
    if(length != 0) {
        i = IPC_WITH_LOAD_CHANNELS;
    }
    for(; i>0; ) {
        i--;
        if(ipc_env.available_buffer & (1 << i)) {
            ipc_env.available_buffer &= (~(1<<i));
            found = true;
            break;
        }
    }
    GLOBAL_INT_RESTORE();

    if(found) {
        return i;
    }
    else {
        return 0xff;
    }
}

/*********************************************************************
 * @fn      ipc_alloc_channel
 *
 * @brief   free an allocated channel after finish transmit.
 *
 * @param   chn     - the number of channel which will be free.
 *			
 * @return  None.
 */
__attribute__((section("need_kept"))) void ipc_free_channel(uint8_t chn)
{    
    GLOBAL_INT_DISABLE();
    ipc_env.available_buffer |= (1<<chn);
    ipc_env.tx_callback[chn] = NULL;
    GLOBAL_INT_RESTORE();
}

/*********************************************************************
 * @fn      ipc_insert_msg
 *
 * @brief   start a message or command transmit.
 *
 * @param   chn      - the number of channel will be used.
 *          format   - contains message type or command
 *          length   - contains length of message
 *          callback - This function will be called after this message
 *                     or command is transmitted.
 *			
 * @return  None.
 */
__attribute__((section("need_kept"))) void ipc_insert_msg(uint8_t chn, uint8_t format, uint16_t length, ipc_tx_callback callback)
{
    struct ipc_msg_t msg;

    msg.length = length;
    msg.format = format;
    msg.tog = chn;
    msg.tag = 1;

    ipc_env.tx_callback[chn] = callback;

    *(uint32_t *)&ipc->msg_out[chn] = *(uint32_t *)&msg;
}

/*********************************************************************
 * @fn      ipc_clear_msg
 *
 * @brief   used to notify the other core a specified message has been 
 *          processed
 *
 * @param   chn     - the index of message.
 *			
 * @return  None.
 */
__attribute__((section("need_kept"))) void ipc_clear_msg(uint8_t chn)
{
    ipc->msg_in[chn].tag = 0;
}

/*********************************************************************
 * @fn      ipc_get_buffer_offset
 *
 * @brief   get buffer offset
 *
 * @param   type    - the direction of tramsmit, reference @ipc_dir_t.
 *          index   - the index of message.
 *			
 * @return  buffer offset.
 */
__attribute__((section("need_kept"))) uint8_t *ipc_get_buffer_offset(uint8_t type, uint8_t index)
{
    if(index >= IPC_WITH_LOAD_CHANNELS) {
        return NULL;
    }

    return (uint8_t *)ipc_buffer_offset[type][index];
}

/*********************************************************************
 * @fn      ipc_dma_rx_int_clear
 *
 * @brief   clear ipc dma rx interrupt
 *
 * @param   None
 *			
 * @return  None
 */
__attribute__((section("need_kept"))) void ipc_dma_rx_int_clear(void)
{
    ipc->dma_ctrl.mcu2dsp_int_clear = 1;
}

/*********************************************************************
 * @fn      ipc_dma_tx_int_clear
 *
 * @brief   clear ipc dma tx interrupt
 *
 * @param   None
 *			
 * @return  None
 */
__attribute__((section("need_kept"))) void ipc_dma_tx_int_clear(void)
{
    ipc->dma_ctrl.dsp2mcu_int_clear = 1;
}

/*********************************************************************
 * @fn      ipc_dma_esco_state_get
 *
 * @brief   get the latest esco packet state
 *
 * @param   None
 *			
 * @return  0: packet is received successfully.
 *          1: packet is not received in reserved slots.
 */
__attribute__((section("need_kept"))) uint8_t ipc_dma_esco_state_get(void)
{
    return (ipc->esco_pkt_sta.esco_pkt_sta != 0);
}

/*********************************************************************
 * @fn      ipc_dma_init
 *
 * @brief   intialize ipc dma mode for voice algorithm
 *
 * @param   rx      - dma rx interrupt handler
 *          tx      - dma tx interrupt handler
 *			
 * @return  None
 */
__attribute__((section("need_kept"))) void ipc_dma_init(ipc_dma_isr_callback rx, ipc_dma_isr_callback tx)
{
    if(rx) {
        ipc->dma_ctrl.mcu2dsp_int_en = 1;
        _xtos_set_interrupt_handler(XCHAL_IPC_DMA_RX_INTERRUPT, rx);
    }

    if(tx) {
        ipc->dma_ctrl.dsp2mcu_int_en = 1;
        _xtos_set_interrupt_handler(XCHAL_IPC_DMA_TX_INTERRUPT, tx);
    }
}

/*********************************************************************
 * @fn      ipc_get_codec_dac_tog
 *
 * @brief   get the current toggle used by codec dac DMA transmit
 *
 * @param   None
 *			
 * @return  current codec_dac_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_codec_dac_tog(void)
{
    return ipc->tog_info.codec_dac_tog;
}

/*********************************************************************
 * @fn      ipc_get_i2s_dac_tog
 *
 * @brief   get the current toggle used by codec dac DMA transmit
 *
 * @param   None
 *			
 * @return  current codec_dac_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_i2s_dac_tog(void)
{
    return ipc->tog_info.i2s_dac_tog;
}

/*********************************************************************
 * @fn      ipc_get_esco_bb2dsp_tog
 *
 * @brief   get the current toggle used by esco transmit from bb to dsp
 *
 * @param   chn     - 0: esco0 channel, 1: esco1 channel
 *			
 * @return  current esco_bb2dsp_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_esco_bb2dsp_tog(uint8_t chn)
{
    if(chn == 0) {
        return ipc->tog_info.esco0_des_tog;
    }
    else if(chn == 1) {
        return ipc->tog_info.esco1_des_tog;
    }
    else {
        return 0;
    }
}

/*********************************************************************
 * @fn      ipc_get_esco_dsp2bb_tog
 *
 * @brief   get the current toggle used by esco transmit from dsp to bb
 *
 * @param   chn     - 0: esco0 channel, 1: esco1 channel
 *			
 * @return  current esco_dsp2bb_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_esco_dsp2bb_tog(uint8_t chn)
{
    if(chn == 0) {
        return ipc->tog_info.esco0_src_tog;
    }
    else if(chn == 1) {
        return ipc->tog_info.esco1_src_tog;
    }
    else {
        return 0;
    }
}

/*********************************************************************
 * @fn      ipc_get_codec_adc_tog
 *
 * @brief   get the current toggle used by codec adc transmit
 *
 * @param   None
 *			
 * @return  current codec_adc_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_codec_adc_tog(void)
{
    return ipc->tog_info.codec_adc_tog;
}

/*********************************************************************
 * @fn      ipc_get_pdm_tog
 *
 * @brief   get the current toggle used by pdm transmit from bb to dsp
 *
 * @param   chn     - 0: PDM0, 1: PDM1, 2: PDM2, 3: PDM3
 *			
 * @return  current pdm_tog
 */
__attribute__((section("need_kept"))) uint8_t ipc_get_pdm_tog(uint8_t chn)
{
    if(chn == 0) {
        return ipc->tog_info.pdm0_tog;
    }
    else if(chn == 1) {
        return ipc->tog_info.pdm1_tog;
    }
    else if(chn == 2) {
        return ipc->tog_info.pdm2_tog;
    }
    else if(chn == 3) {
        return ipc->tog_info.pdm3_tog;
    }
    else {
        return 0;
    }
}

/*********************************************************************
 * @fn      ipc_msg_sent
 *
 * @brief   called in interrupt handler after a message has been sent. Sending list
 * 			will be checked inside this function.
 *
 * @param   None
 *
 * @return  None
 */
__attribute__((section("need_kept"))) static void ipc_msg_sent(void)
{
    uint8_t chn;

    if(co_list_is_empty(&ipc_msg_list) == 0) {
        struct ipc_msg_elem_t *ele = (struct ipc_msg_elem_t *)co_list_pick(&ipc_msg_list);
        if(ele->with_payload) {
            chn = ipc_alloc_channel(ele->p.payload.length);
        }
        else {
            chn = ipc_alloc_channel(0);
        }
        if(chn != 0xff) {
            co_list_pop_front(&ipc_msg_list);
            if(ele->with_payload) {
                if(ele->p.payload.length) {
                    uint8_t *buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, chn);
                    memcpy(buffer, ele->p.payload.buffer, ele->p.payload.length);
                    basic_free(ele->p.payload.buffer);
                }
                ipc_insert_msg(chn, ele->msg, ele->p.payload.length, ele->callback);
            }
            else {
                ipc_insert_msg(chn, ele->msg, ele->p.sub_msg, ele->callback);
            }
            basic_free((void *)ele);
        }
    }
}

/*********************************************************************
 * @fn      ipc_msg_send
 *
 * @brief   Send a message without payload to MCU
 *
 * @param   msg		- message type.
 * 			sub_msg	- use length segment in message to indicate sub message type.
 * 			callback- the function to be called after this message has been sent.
 *
 * @return  None
 */
__attribute__((section("need_kept"))) void ipc_msg_send(enum ipc_msg_type_t msg, uint16_t sub_msg, ipc_tx_callback callback)
{
    uint8_t chn;

    GLOBAL_INT_DISABLE();
    chn = ipc_alloc_channel(0);
    if(chn != 0xff) {
        ipc_insert_msg(chn, msg, sub_msg, callback);
    }
    else {
        struct ipc_msg_elem_t *elt;
        elt = (struct ipc_msg_elem_t *)basic_malloc(sizeof(struct ipc_msg_elem_t));
        elt->with_payload = false;
        elt->msg = msg;
        elt->p.sub_msg = sub_msg;
        elt->callback = callback;
        co_list_push_back(&ipc_msg_list, &elt->hdr);
    }
    GLOBAL_INT_RESTORE();
}

/*********************************************************************
 * @fn      ipc_msg_send
 *
 * @brief   Send a message with payload to MCU
 *
 * @param   msg		- message type.
 * 			header	- ipc message is composite of header and payload. The content of
 * 					  header will be copied into share memory.
 * 			header_length - length of header.
 * 			payload - The content of payload will be copied into share memory following header.
 * 			payload_length - length of payload
 * 			callback- the function to be called after this message has been sent.
 *
 * @return  None
 */
__attribute__((section("need_kept"))) void ipc_msg_with_payload_send(enum ipc_msg_type_t msg,
																		void *header,
																		uint16_t header_length,
																		uint8_t *payload,
																		uint16_t payload_length,
																		ipc_tx_callback callback)
{
    uint8_t chn;
    uint8_t *ipc_buffer;
    uint16_t total_length = header_length+payload_length;

    GLOBAL_INT_DISABLE();
    chn = ipc_alloc_channel(total_length);
    if(chn != 0xff) {
        ipc_buffer = ipc_get_buffer_offset(IPC_DIR_DSP2MCU, chn);
        memcpy(ipc_buffer, header, header_length);
        memcpy(ipc_buffer+header_length, payload, payload_length);
        ipc_insert_msg(chn, msg, total_length, callback);
    }
    else {
        struct ipc_msg_elem_t *elt;
        elt = (struct ipc_msg_elem_t *)basic_malloc(sizeof(struct ipc_msg_elem_t));
        elt->with_payload = true;
        elt->msg = msg;
        if(total_length) {
            uint8_t *tmp_buffer;
            tmp_buffer = basic_malloc(total_length);
            memcpy(tmp_buffer, header, header_length);
            memcpy(tmp_buffer+header_length, payload, payload_length);
            elt->p.payload.buffer = tmp_buffer;
        }
        elt->p.payload.length = total_length;
        elt->callback = callback;
        co_list_push_back(&ipc_msg_list, &elt->hdr);
    }
    GLOBAL_INT_RESTORE();
}

__attribute__((section("need_kept"))) void ipc_isr(void)
{
    uint32_t status;
    struct ipc_msg_t msg;
    ipc_tx_callback callback;

    status = *(volatile uint32_t *)&ipc->ctr;
    *(volatile uint32_t *)&ipc->ctr = status;    // clear interrupt

    if(status & IPC_MSGOUT00_STATUS) {
    	callback = ipc_env.tx_callback[0];
		ipc_free_channel(0);
		ipc_msg_sent();
		if(callback) {
			callback(0);
        }
    }
    if(status & IPC_MSGOUT01_STATUS) {
    	callback = ipc_env.tx_callback[1];
		ipc_free_channel(1);
		ipc_msg_sent();
		if(callback) {
			callback(1);
        }
    }
    if(status & IPC_MSGOUT10_STATUS) {
    	callback = ipc_env.tx_callback[2];
		ipc_free_channel(2);
		ipc_msg_sent();
		if(callback) {
			callback(2);
        }
    }
    if(status & IPC_MSGOUT11_STATUS) {
    	callback = ipc_env.tx_callback[3];
		ipc_free_channel(3);
		ipc_msg_sent();
		if(callback) {
			callback(3);
        }
    }

    if(status & IPC_MSGIN00_STATUS) {
        if(ipc_env.rx_callback) {
            *(uint32_t *)&msg = *(uint32_t *)&ipc->msg_in[0];
            ipc_env.rx_callback(&msg, 0);
        }
    }
    if(status & IPC_MSGIN01_STATUS) {
        if(ipc_env.rx_callback) {
            *(uint32_t *)&msg = *(uint32_t *)&ipc->msg_in[1];
            ipc_env.rx_callback(&msg, 1);
        }
    }
    if(status & IPC_MSGIN10_STATUS) {
        if(ipc_env.rx_callback) {
            *(uint32_t *)&msg = *(uint32_t *)&ipc->msg_in[2];
            ipc_env.rx_callback(&msg, 2);
        }
    }
    if(status & IPC_MSGIN11_STATUS) {
        if(ipc_env.rx_callback) {
            *(uint32_t *)&msg = *(uint32_t *)&ipc->msg_in[3];
            ipc_env.rx_callback(&msg, 3);
        }
    }
}
