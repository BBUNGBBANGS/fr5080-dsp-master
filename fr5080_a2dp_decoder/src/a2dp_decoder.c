#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <xtensa/xtruntime.h>

#include "tasks.h"
#include "co_mem.h"

#include "plf.h"
#include "ipc.h"
#include "uart.h"
#include "tasks.h"

char char2ascii[] = "0123456789abcdef";

static void a2dp_decoder_ipc_rx(void)
{
    int16_t *mic;

    ipc_dma_rx_int_clear();

    //mic from codec adc
    if(ipc_get_codec_adc_tog()) {
        mic = (int16_t *)IPC_CODEC_READ_BUFFER0;
    }
    else {
        mic = (int16_t *)IPC_CODEC_READ_BUFFER1;
    }

    for(uint32_t i=0; i<60; i++) {
        int16_t value = *mic++;
        uart_putc_noint(char2ascii[(value>>12)&0x0f]);
        uart_putc_noint(char2ascii[(value>>8)&0x0f]);
        uart_putc_noint(char2ascii[(value>>4)&0x0f]);
        uart_putc_noint(char2ascii[(value)&0x0f]);
    }
    uart_putc_noint('\r');
    uart_putc_noint('\n');
}

static void a2dp_decoder_ipc_tx(void)
{
    uint8_t *spk;

    ipc_dma_tx_int_clear();
    uart_putc_noint('T');

    if(ipc_get_codec_dac_tog() == 0) {
        spk = (uint8_t *)IPC_DSP_WRITE_BUFFER1;
    }
    else {
        spk = (uint8_t *)IPC_DSP_WRITE_BUFFER0;
    }

    // fill spk with 512 bytes pcm data
}

void a2dp_decoder_recv_frame(uint8_t *data, uint32_t length)
{
    printf("length is %d\r\n", length);
}

void a2dp_decoder_init(void *msg)
{
    uart_puts_noint("audio init\r\n");

    ipc_dma_init(a2dp_decoder_ipc_rx, a2dp_decoder_ipc_tx);

    _xtos_interrupt_enable(XCHAL_IPC_DMA_RX_INTERRUPT);
    _xtos_interrupt_enable(XCHAL_IPC_DMA_TX_INTERRUPT);
}

void a2dp_decoder_release(void *msg)
{
}
