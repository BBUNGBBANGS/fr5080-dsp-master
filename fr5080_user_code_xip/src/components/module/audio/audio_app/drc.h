/*
 * drc.h
 *
 *  Created on: 2022Äê5ÔÂ16ÈÕ
 *      Author: owen
 */

#ifndef _DRC_H
#define _DRC_H

#include <stdint.h>

struct drc_param {
	int16_t initial_gain;
	int16_t target_threshold;
};

void drc_init(struct drc_param *param);
void drc_destroy(void);
void drc_run(int16_t *data, uint32_t length);

#endif /* _DRC_H */
