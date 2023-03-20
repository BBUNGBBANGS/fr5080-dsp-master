/*
 * app_at.c
 *
 *  Created on: 2021-4-29
 *      Author: Administrator
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "tasks.h"

#include "plf.h"

#define AT_RECV_MAX_LEN             32

uint8_t app_at_recv_char;
uint8_t at_recv_buffer[AT_RECV_MAX_LEN];
uint8_t at_recv_index = 0;
uint8_t at_recv_state = 0;

/*-------------------------------------------------------------------------
    Function    :  ascii_char2val             ----add by chsheng, chsheng@accelsemi.com
    Return: -1=error
    Description:
        'a' -> 0xa  'A' -> 0xa
-------------------------------------------------------------------------*/
static char ascii_char2val(const char c)
{
    if(c>='0' && c<='9')
        return c-'0';
    if((c>='a' && c<='f') || (c>='A' && c<='F'))
        return (c&0x7)+9;

    return (char)(-1);
}

/*-------------------------------------------------------------------------
    Function    :  ascii_strn2val             ----add by chsheng, chsheng@accelsemi.com
    Return: -1=error
    Description:
        str = "123" bas = 10 return 123
        str = "123" bas = 16 return 0x123
-------------------------------------------------------------------------*/
static int ascii_strn2val( const char str[], char base, char n)
{
    int val = 0;
    char v;
    while(n != 0){
        v = ascii_char2val(*str);
#if 0
        if (v == -1 || v >= base)
            return -1;
#else
        if (v == (char)(-1) || v >= base)
        {
            if(val == 0) //to filter abormal beginning and ending
            {
                str ++;
                n --;
                continue;
            }
            else
            {
                break;
            }
        }
#endif
        val = val*base + v;

        str++;
        n--;
    }
    return val;
}

static void app_at_recv_cmd_A(uint8_t sub_cmd, uint8_t *data)
{
    //struct bd_addr addr;
    //uint8_t tmp_data = 0;
    uint32_t data0, data1, data2;

    switch(sub_cmd)
    {
        case 'A':
            GLOBAL_INT_DISABLE();
            {
                uint8_t counter = ascii_strn2val((const char *)&data[0], 10, 2);
                uint8_t counter_single = ascii_strn2val((const char *)&data[3], 10, 2);
                while(counter--) {
                    void native_playback_ipc_check_irq(void);
                    co_delay_100us(counter_single);
                    native_playback_ipc_check_irq();
                }
            }
            GLOBAL_INT_RESTORE();
            break;
        case 'G':
            printf("hello world!\r\n");
            break;
        case 'H':
            printf("VAL: 0x%08x.\r\n", *(uint32_t *)(ascii_strn2val((const char *)&data[0], 16, 8)));
            break;
        case 'I':
            *(uint32_t *)ascii_strn2val((const char *)&data[0], 16, 8) = ascii_strn2val((const char *)&data[9], 16, 8);
            printf("OK\r\n");
            break;
        case 'U':
            {
                uint32_t *ptr = (uint32_t *)(ascii_strn2val((const char *)&data[0], 16, 8) & (~3));
                uint8_t count = ascii_strn2val((const char *)&data[9], 16, 2);
                uint32_t *start = (uint32_t *)((uint32_t)ptr & (~0x0f));
                for(uint8_t i=0; i<count;) {
                    if(((uint32_t)start & 0x0c) == 0) {
                        printf("0x%08x: ", start);
                    }
                    if(start < ptr) {
                        printf("        ");
                    }
                    else {
                        i++;
                        printf("%08x", *start);
                    }
                    if(((uint32_t)start & 0x0c) == 0x0c) {
                        printf("\r\n");
                    }
                    else {
                        printf(" ");
                    }
                    start++;
                }
                printf("\r\n");
            }
            break;
    }
}

void app_at_cmd_recv_handler(struct task_msg_t *msg)
{
    uint8_t *data = (void *)&msg->param[0];
    switch(data[0])
    {
        case 'A':
            app_at_recv_cmd_A(data[1], &data[2]);
            break;
            break;
    }
}

void uart_receive_char(uint8_t c)
{
    switch(at_recv_state)
    {
        case 0:
            if(c == 'A')
            {
                at_recv_state++;
            }
            break;
        case 1:
            if(c == 'T')
                at_recv_state++;
            else
                at_recv_state = 0;
            break;
        case 2:
            if(c == '#')
                at_recv_state++;
            else
                at_recv_state = 0;
            break;
        case 3:
            at_recv_buffer[at_recv_index++] = c;
            if((c == '\n')
               ||(at_recv_index >= AT_RECV_MAX_LEN))
            {
                struct task_msg_t *msg = task_msg_alloc(RECEIVE_AT_COMMAND, at_recv_index);
                memcpy((void *)&msg->param[0], at_recv_buffer, at_recv_index);
                task_msg_insert(msg);

                at_recv_state = 0;
                at_recv_index = 0;
            }
            break;
    }
}
