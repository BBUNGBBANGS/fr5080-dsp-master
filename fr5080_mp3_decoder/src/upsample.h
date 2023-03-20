/*
 * upsample.h
 *
 *  Created on: 2021-6-2
 *      Author: Administrator
 */

#ifndef _UPSAMPLE_H
#define _UPSAMPLE_H

#include <stdint.h>

typedef void (*upsample_callback)(uint8_t *data, int32_t length);

void upsample_init(void);
void upsample_destroy(void);
void upsample_entity(uint8_t *indata, int32_t insize, upsample_callback callback);

#endif /* _UPSAMPLE_H */
