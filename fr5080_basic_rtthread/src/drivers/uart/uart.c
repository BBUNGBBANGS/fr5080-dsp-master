#include <stddef.h>
#include <stdio.h>

#include <xtensa/xtruntime.h>

#include "plf.h"
#include "uart.h" 

#define REG_PL_WR(addr, data)       *(volatile uint32_t *)(addr) = (data)
#define REG_PL_RD(addr)             *(volatile uint32_t *)(addr)

#define UART_CLK                    14745600
#define UART_FIFO_TRIGGER           (UART_FIFO_RX_TRIGGER|UART_FIFO_TX_TRIGGER)

const uint16_t uart_baud_map[12] = {
	12,24,48,96,144,192,384,576,1152,2304,4608,9216
};

volatile struct uart_reg_t * const uart_reg = (volatile struct uart_reg_t *)UART_BASE;

void (*uart_callback)(uint8_t c) = NULL;

int uart_get_baud_divisor(uint8_t baudrate)
{
	int Tmp32;
    
	Tmp32 = uart_baud_map[baudrate]*100;

	return (UART_CLK/(16*Tmp32));
}

void uart_reset_register(void)
{
    volatile uint32_t misc_data;

    REG_PL_WR(&(uart_reg->u2.ier),0);
    REG_PL_WR(&(uart_reg->u3.fcr),0x06);
    REG_PL_WR(&(uart_reg->lcr),0);

    misc_data = REG_PL_RD(&(uart_reg->lsr));
    misc_data = REG_PL_RD(&(uart_reg->mcr));
}

void uart_init(uint8_t bandrate, void (*callback)(uint8_t))
{
    int uart_baud_divisor;

    /* wait for tx fifo is empty */
    while(!(uart_reg->lsr & 0x40));

    uart_reset_register();

    uart_baud_divisor = uart_get_baud_divisor(bandrate);

    while(!(uart_reg->lsr & 0x40));

    /* baud rate */
    uart_reg->lcr.divisor_access = 1;
    uart_reg->u1.dll.data = uart_baud_divisor & 0xff;
    uart_reg->u2.dlm.data = (uart_baud_divisor>>8) & 0xff;
    uart_reg->lcr.divisor_access = 0;

    /*word len*/
    uart_reg->lcr.word_len = 3;
    uart_reg->lcr.parity_enable = 0;
    uart_reg->lcr.even_enable = 0;
    uart_reg->lcr.stop = 0;

    /*fifo*/
    uart_reg->u3.fcr.data = UART_FIFO_TRIGGER | FCR_FIFO_ENABLE;

    /*auto flow control*/
    uart_reg->mcr.afe = 0;

    /*enable recv and line status interrupt*/
    uart_reg->u2.ier.erdi = 1;
    uart_reg->u2.ier.erlsi = 1;

    if(callback) {
        uart_callback = callback;
    }
}

void uart_flush_rxfifo_noint(void)
{
	volatile uint8_t data;

	while(uart_reg->lsr & 0x01) {
		data = uart_reg->u1.data;
	}
}

void uart_finish_transfers(void)
{
    /* wait for tx fifo is empty */
    while(!(uart_reg->lsr & 0x40));
}

void uart_putc_noint(uint8_t c)
{
    while (!(uart_reg->lsr & 0x40));
    uart_reg->u1.data = c;
}

void uart_puts_noint(const char *s)
{
	uint32_t count;

	while (1) {
		while (!(uart_reg->lsr & 0x40));

		count=UART_FIFO_SIZE;

		while (count--&&*s) {
			uart_reg->u1.data = *s++;
		}

		if (*s=='\0') {
			return;
		}
	}
}

void uart_putc_noint_no_wait(uint8_t c)
{
    uart_reg->u1.data = c;
}

void uart_put_data_noint(const uint8_t *d, int size)
{
	uint32_t count;

	while (1) {
		while (!(uart_reg->lsr & 0x40));

		count = UART_FIFO_SIZE;

		while((count--)&&(size!=0)) {
			uart_reg->u1.data = *d++;
			size--;
		}

		if(size == 0) {
			return;
		}
	}
}

void uart_get_data_noint(uint8_t *buf, int size)
{
	while (1) {
		while ((uart_reg->lsr & 0x01)&&(size!=0)) {
			*buf++ = uart_reg->u1.data;
			size--;
		}

		if (size==0) {
			return;
		}

		while(!(uart_reg->lsr & 0x01));
	}
}

int uart_get_data_nodelay_noint(uint8_t *buf, int size)
{
	int count = 0;

	if(uart_reg->lsr & 0x01) {
		while((uart_reg->lsr & 0x01) && (size != 0)) {
			*buf++ = uart_reg->u1.data;
			size--;
			count++;
		}

	}

	return count;
}

__attribute__((section("need_kept"))) void uart_isr(void)
{
    uint8_t int_id;
    volatile uint8_t c;

    int_id = uart_reg->u3.iir.int_id;

    if(int_id == 0x04 || int_id == 0x0c ) { /* Receiver data available or Character time-out indication */
        #if 0
        while(uart_reg->lsr & 0x01) {
            c = uart_reg->u1.data;
            if(uart_callback) {
                uart_callback(c);
            }
        }
        #else
        if(uart_callback) {
            uart_callback(0);
        }
        else {
            while(uart_reg->lsr & 0x01) {
                c = uart_reg->u1.data;
            }
        }
        #endif
    }
    else if(int_id == 0x06)
    {
        volatile uint32_t line_status = uart_reg->lsr;
    }
}

void uart_write(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t))
{
    uint32_t count;

    while (1) {
        uart_finish_transfers();

        count = UART_FIFO_SIZE;

        while (count--&&size!=0) {
            uart_reg->u1.data = *bufptr++;				
            size--;
        }

        if (size==0) {
            break;
        }
    }

    uart_finish_transfers();

    if(callback) {
        callback(0);
    }
}
