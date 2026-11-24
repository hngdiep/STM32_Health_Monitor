
#ifndef BPM_H
#define BPM_H

#include <stdint.h>

extern volatile uint32_t g_bpm_x100;
extern volatile uint32_t g_bpm_valid;
extern volatile uint32_t g_bpm_detected_peaks;
extern volatile uint32_t g_bpm_last_interval_samples;

void BPM_Init(void);
void BPM_ProcessIR(uint32_t ir_raw);

#endif
