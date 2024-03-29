/*
 * Copyright (C) 2016 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */

/*
 * cvsd_plc.h
 *
 */

#ifndef __BTSTACK_CVSD_PLC_H
#define __BTSTACK_CVSD_PLC_H

#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

#define CVSD_FS 30           /* CVSD Frame Size */
#define CVSD_N 128           /* 16ms - Window Length for pattern matching */ 
#define CVSD_M 10            /* Template for matching */
#define CVSD_LHIST (CVSD_N+CVSD_FS-1)  /* Length of history buffer required */ 
#define CVSD_RT 5        /*  Reconvergence Time (samples) */
#define CVSD_OLAL 16         /* OverLap-Add Length (samples) */

/* PLC State Information */
typedef struct cvsd_plc_state {
    int16_t hist[CVSD_LHIST+CVSD_FS+CVSD_RT+CVSD_OLAL];
    int16_t bestlag;
    int     nbf;

    // summary of processed good and bad frames
    int good_frames_nr;
    int bad_frames_nr;
    int frame_count;
} cvsd_plc_state_t;

// All int16 audio samples are in host endiness
void cvsd_plc_init(cvsd_plc_state_t *plc_state);
void cvsd_plc_bad_frame(cvsd_plc_state_t *plc_state, int16_t *out); 
void cvsd_plc_good_frame(cvsd_plc_state_t *plc_state, int16_t *in);
void cvsd_plc_process_data(cvsd_plc_state_t * state, int16_t * in, uint16_t size, int16_t * out);
void cvsd_dump_statistics(cvsd_plc_state_t * state);

#if defined __cplusplus
}
#endif

#endif // __BTSTACK_CVSD_PLC_H
