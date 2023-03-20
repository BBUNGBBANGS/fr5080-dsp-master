#ifndef _CO_UTIL_H
#define _CO_UTIL_H

#include <stdint.h>

int ascii_strn2val( const char str[], char base, char n);
void co_delay_100us(uint32_t count);
void co_delay_10us(uint32_t count);

#endif //_CO_UTIL_H
