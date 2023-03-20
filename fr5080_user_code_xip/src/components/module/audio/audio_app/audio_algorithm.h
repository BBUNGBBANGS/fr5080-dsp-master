#ifndef _AUDIO_ALGORITHM_H
#define _AUDIO_ALGORITHM_H

#include <stdint.h>

void audio_algorithm_init(void);
void audio_algorithm_release(void *);
void audio_algorithm_recv_data(int16_t *mic, int16_t *esco_in, uint8_t esco_data_mute);
void audio_algorithm_send_data(int16_t *spk, int16_t *spk_i2s, int16_t *esco_out);

#endif  //_AUDIO_ALGORITHM_H
