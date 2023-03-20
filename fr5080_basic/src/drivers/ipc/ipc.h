/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */
#ifndef _IPC_H
#define _IPC_H

/*
 * INCLUDES
 */
#include <stdint.h>

/*
 * TYPEDEFS
 */
#define IPC_DSP_READ_BUFFER0        0x001F0000
#define IPC_DSP_READ_BUFFER1        0x001F0400
#define IPC_DSP_WRITE_BUFFER0       0x001F1000
#define IPC_DSP_WRITE_BUFFER1       0x001F1400
#define IPC_DSP_READ_OPT_BUFFER0    0x001F0800
#define IPC_DSP_READ_OPT_BUFFER1    0x001F0A00
#define IPC_DSP_WRITE_OPT_BUFFER0   0x001F1800
#define IPC_DSP_WRITE_OPT_BUFFER1   0x001F1A00

#define IPC_CODEC_READ_BUFFER0      (IPC_DSP_READ_BUFFER0+0)
#define IPC_CODEC_READ_BUFFER1      (IPC_DSP_READ_BUFFER0+120)
#define IPC_ESCO0_READ_BUFFER0      (IPC_DSP_READ_BUFFER0+240)
#define IPC_ESCO0_READ_BUFFER1      (IPC_DSP_READ_BUFFER0+360)
#define IPC_PDM0_L_READ_BUFFER0     (IPC_DSP_READ_BUFFER0+480)
#define IPC_PDM0_L_READ_BUFFER1     (IPC_DSP_READ_BUFFER0+600)
#define IPC_PDM0_R_READ_BUFFER0     (IPC_DSP_READ_BUFFER0+720)
#define IPC_PDM0_R_READ_BUFFER1     (IPC_DSP_READ_BUFFER0+840)
#define IPC_PDM1_L_READ_BUFFER0     (IPC_DSP_READ_BUFFER0+960)
#define IPC_PDM1_L_READ_BUFFER1     (IPC_DSP_READ_BUFFER0+1080)
#define IPC_PDM1_R_READ_BUFFER0     (IPC_DSP_READ_BUFFER0+1200)
#define IPC_PDM1_R_READ_BUFFER1     (IPC_DSP_READ_BUFFER0+1320)
#define IPC_ESCO1_READ_BUFFER0      (IPC_DSP_READ_BUFFER0+1440)
#define IPC_ESCO1_READ_BUFFER1      (IPC_DSP_READ_BUFFER0+1560)
#define IPC_I2S_READ_BUFFER0      	(IPC_DSP_READ_BUFFER0+1680)
#define IPC_I2S_READ_BUFFER1      	(IPC_DSP_READ_BUFFER0+1800)

#define IPC_CODEC_WRITE_BUFFER0     (IPC_DSP_WRITE_BUFFER0+0)
#define IPC_CODEC_WRITE_BUFFER1     (IPC_DSP_WRITE_BUFFER0+120)
#define IPC_ESCO0_WRITE_BUFFER0     (IPC_DSP_WRITE_BUFFER0+240)
#define IPC_ESCO0_WRITE_BUFFER1     (IPC_DSP_WRITE_BUFFER0+360)
#define IPC_ESCO1_WRITE_BUFFER0     (IPC_DSP_WRITE_BUFFER0+480)
#define IPC_ESCO1_WRITE_BUFFER1     (IPC_DSP_WRITE_BUFFER0+600)
#define IPC_I2S_WRITE_BUFFER0       (IPC_DSP_WRITE_BUFFER0+720)
#define IPC_I2S_WRITE_BUFFER1       (IPC_DSP_WRITE_BUFFER0+840)

enum ipc_msg_type_t {
    IPC_MSG_LOAD_CODE,
    IPC_MSG_LOAD_CODE_DONE,
    IPC_MSG_EXEC_USER_CODE,
    IPC_MSG_DSP_READY = 10,
};

enum ipc_dir_t {
    IPC_DIR_MCU2DSP,
    IPC_DIR_DSP2MCU,
};

enum ipc_int_t {
    IPC_MSGIN00_INT = (1<<0),
    IPC_MSGIN01_INT = (1<<1),
    IPC_MSGIN10_INT = (1<<2),
    IPC_MSGIN11_INT = (1<<3),
    IPC_MSGOUT00_INT = (1<<4),
    IPC_MSGOUT01_INT = (1<<5),
    IPC_MSGOUT10_INT = (1<<6),
    IPC_MSGOUT11_INT = (1<<7),
};

enum ipc_int_status_t {
    IPC_MSGIN00_STATUS = (1<<8),
    IPC_MSGIN01_STATUS = (1<<9),
    IPC_MSGIN10_STATUS = (1<<10),
    IPC_MSGIN11_STATUS = (1<<11),
    IPC_MSGOUT00_STATUS = (1<<12),
    IPC_MSGOUT01_STATUS = (1<<13),
    IPC_MSGOUT10_STATUS = (1<<14),
    IPC_MSGOUT11_STATUS = (1<<15),
};

struct ipc_msg_load_code_t {
    uint32_t dest;
    uint32_t data[0];
};

struct ipc_msg_exec_user_code_t {
    uint32_t *entry_ptr;
};

struct ipc_msg_qspi_init_param_t {
	uint32_t cache_val;
    uint8_t type;	//1,2,4
    uint8_t div;
    uint8_t fast_read_enable;
    uint8_t qspi_sel;
};

struct ipc_msg_media_t {
    uint32_t sample_rate;
    uint8_t media_type;
    uint8_t channel_num;
};

struct ipc_msg_t {
    uint32_t length:10;
    uint32_t format:4;
    uint32_t tog:1;
    uint32_t tag:1;
    uint32_t reserved:16;
};

typedef void (*ipc_tx_callback)(uint8_t chn);
typedef void (*ipc_rx_callback)(struct ipc_msg_t *msg, uint8_t chn);
typedef void (*ipc_dma_isr_callback)(void);

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
void ipc_init(uint32_t ints_en, ipc_rx_callback callback);

/*********************************************************************
 * @fn      ipc_alloc_channel
 *
 * @brief   allocate a channel for following transmit.
 *
 * @param   length      - the length of message. If this value is 0,
 *                      - channel 2 and 3 are also available.
 *			
 * @return  allocated channel number.
 */
uint8_t ipc_alloc_channel(uint16_t length);

/*********************************************************************
 * @fn      ipc_alloc_channel
 *
 * @brief   free an allocated channel after finish transmit.
 *
 * @param   chn     - the number of channel which will be free.
 *			
 * @return  None.
 */
void ipc_free_channel(uint8_t chn);

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
void ipc_insert_msg(uint8_t chn, uint8_t format, uint16_t length, ipc_tx_callback callback);

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
void ipc_clear_msg(uint8_t chn);

/*********************************************************************
 * @fn      ipc_get_buffer_offset
 *
 * @brief   get buffer offset
 *
 * @param   type    - the direction of tramsmit, reference @ipc_dir_t.
 *          index   - the index of message.
 *			
 * @return  None.
 */
uint8_t *ipc_get_buffer_offset(uint8_t type, uint8_t index);

/*********************************************************************
 * @fn      ipc_dma_rx_int_clear
 *
 * @brief   clear ipc dma rx interrupt
 *
 * @param   None
 *			
 * @return  None
 */
void ipc_dma_rx_int_clear(void);

/*********************************************************************
 * @fn      ipc_dma_tx_int_clear
 *
 * @brief   clear ipc dma tx interrupt
 *
 * @param   None
 *			
 * @return  None
 */
void ipc_dma_tx_int_clear(void);

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
uint8_t ipc_dma_esco_state_get(void);

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
void ipc_dma_init(ipc_dma_isr_callback rx, ipc_dma_isr_callback tx);

/*********************************************************************
 * @fn      ipc_get_codec_dac_tog
 *
 * @brief   get the current toggle used by codec dac DMA transmit
 *
 * @param   rx      - dma rx interrupt handler
 *          tx      - dma tx interrupt handler
 *			
 * @return  None
 */
uint8_t ipc_get_codec_dac_tog(void);

/*********************************************************************
 * @fn      ipc_get_i2s_dac_tog
 *
 * @brief   get the current toggle used by codec dac DMA transmit
 *
 * @param   None
 *			
 * @return  current codec_dac_tog
 */
uint8_t ipc_get_i2s_dac_tog(void);

/*********************************************************************
 * @fn      ipc_get_esco_bb2dsp_tog
 *
 * @brief   get the current toggle used by esco transmit from bb to dsp
 *
 * @param   chn     - 0: esco0 channel, 1: esco1 channel
 *			
 * @return  current esco_bb2dsp_tog
 */
uint8_t ipc_get_esco_bb2dsp_tog(uint8_t chn);

/*********************************************************************
 * @fn      ipc_get_esco_dsp2bb_tog
 *
 * @brief   get the current toggle used by esco transmit from dsp to bb
 *
 * @param   chn     - 0: esco0 channel, 1: esco1 channel
 *			
 * @return  current esco_dsp2bb_tog
 */
uint8_t ipc_get_esco_dsp2bb_tog(uint8_t chn);

/*********************************************************************
 * @fn      ipc_get_codec_adc_tog
 *
 * @brief   get the current toggle used by codec adc transmit
 *
 * @param   None
 *			
 * @return  current codec_adc_tog
 */
uint8_t ipc_get_codec_adc_tog(void);

/*********************************************************************
 * @fn      ipc_get_pdm_tog
 *
 * @brief   get the current toggle used by pdm transmit from bb to dsp
 *
 * @param   chn     - 0: PDM0, 1: PDM1, 2: PDM2, 3: PDM3
 *			
 * @return  current pdm_tog
 */
uint8_t ipc_get_pdm_tog(uint8_t chn);

/*********************************************************************
 * @fn      ipc_get_i2s_adc_tog
 *
 * @brief   get the current toggle used by i2s adc transmit
 *
 * @param   None
 *			
 * @return  current i2s_adc_tog
 */
uint8_t ipc_get_i2s_adc_tog(void);

#endif  // _IPC_H

