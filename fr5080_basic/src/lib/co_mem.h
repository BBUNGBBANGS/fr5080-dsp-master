#ifndef _CO_MEM_H
#define _CO_MEM_H

#include <stddef.h>

void *pvPortMalloc( size_t xWantedSize );
void vPortFree( void *pv );

#endif  // _CO_MEM_H
