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
    // 拷贝只读数据到dram
    for(ptr = &_rodata_start, src = &_rodata_load_addr; ptr < &_rodata_end;) {
        *ptr++ = *src++;
    }
    // 拷贝数据的初始值到dram
    for(ptr = &_data_start, src = &_data_load_addr; ptr < &_data_end;) {
        *ptr++ = *src++;
    }
    // 将中断、需要快速响应等程序拷贝到iram中，关键代码通过__attribute__((section("iram_section")))修饰
    for(ptr = &_iram_text_start, src = &_iram_text_load_addr; ptr < &_iram_text_end;) {
        *ptr++ = *src++;
    }
#endif

    // 将bss段初始化为0
    for(ptr = &_bss_start; ptr < & _bss_end;) {
        *ptr++ = 0;
    }

    extern void *(*basic_malloc)( uint32_t );
    extern void (*basic_free)( void * );
    basic_malloc = pvPortMalloc;
    basic_free = vPortFree;
}
