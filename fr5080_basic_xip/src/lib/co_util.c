#include <stdint.h>

#include <xtensa/hal.h>
#include <xtensa/core-macros.h>

#include "plf.h"

void *(*basic_malloc)( size_t xWantedSize ) = NULL;
void (*basic_free)( void *pv ) = NULL;

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
int ascii_strn2val( const char str[], char base, char n)
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

#define system_clock                156
#define FRQ_HIFI3_10US_COUNT        (system_clock*10)
void co_delay_10us(uint32_t count)
{
    uint32_t begin_count;

    GLOBAL_INT_DISABLE();
    xthal_interrupt_clear(XCHAL_TIMER0);
    begin_count = xthal_get_ccount();
    xthal_set_ccompare(0, begin_count + count * FRQ_HIFI3_10US_COUNT);
    GLOBAL_INT_RESTORE();
    while((xthal_get_interrupt() & (1<<XCHAL_TIMER0)) == 0);
}

void co_delay_100us(uint32_t count)
{
    co_delay_10us(count * 10);
}

