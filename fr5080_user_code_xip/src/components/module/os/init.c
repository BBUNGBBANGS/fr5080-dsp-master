/*
 * init.c
 *
 *  Created on: 2019-1-1
 *      Author: Administrator
 */

#include <stdint.h>

#include "co_mem.h"

extern uint32_t _bss_start, _bss_end;
extern uint32_t _rodata_start, _rodata_end;
extern uint32_t _rodata_load_addr, _data_load_addr, _iram_text_load_addr;
extern uint32_t _data_start, _iram_text_start;
extern uint32_t _data_end, _iram_text_end;

void init_memory(void)
{
	uint32_t *ptr, *src;

#if 1
    // ����ֻ�����ݵ�dram
    for(ptr = &_rodata_start, src = &_rodata_load_addr; ptr < &_rodata_end;) {
        *ptr++ = *src++;
    }
    // �������ݵĳ�ʼֵ��dram
    for(ptr = &_data_start, src = &_data_load_addr; ptr < &_data_end;) {
        *ptr++ = *src++;
    }
    // ���жϡ���Ҫ������Ӧ�ȳ��򿽱���iram�У��ؼ�����ͨ��__attribute__((section("iram_section")))����
    for(ptr = &_iram_text_start, src = &_iram_text_load_addr; ptr < &_iram_text_end;) {
        *ptr++ = *src++;
    }
#endif

    // ��bss�γ�ʼ��Ϊ0
    for(ptr = &_bss_start; ptr < & _bss_end;) {
        *ptr++ = 0;
    }

    extern void *(*basic_malloc)( uint32_t );
    extern void (*basic_free)( void * );
    basic_malloc = pvPortMalloc;
    basic_free = vPortFree;
}
