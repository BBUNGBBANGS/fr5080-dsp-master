/*
 * downsample.h
 *
 *  Created on: 2018-7-7
 *      Author: Administrator
 */

#ifndef _DOWNSAMPLE_H
#define _DOWNSAMPLE_H

#include <stdint.h>

typedef void (*downsample_callback)(uint8_t *data, int32_t length);

void downsample_init(void);
void downsample_destroy(void);
void downsample_entity(uint8_t *indata, int32_t insize, downsample_callback callback);

#endif /* _DOWNSAMPLE_H */
