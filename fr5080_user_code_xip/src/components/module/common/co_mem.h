#ifndef _CO_MEM_H
#define _CO_MEM_H

#include <stddef.h>

void *pvPortMalloc( size_t xWantedSize );
void vPortFree( void *pv );

void *pvPortMalloc_user( size_t xWantedSize );
void vPortFree_user( void *pv );

#endif  // _CO_MEM_H
