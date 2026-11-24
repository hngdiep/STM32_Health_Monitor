
#ifndef SPO2_H
#define SPO2_H

#include <stdint.h>

extern volatile uint32_t g_spo2_ratio_x10000;
extern volatile uint32_t g_spo2_demo_x100;
extern volatile uint32_t g_spo2_valid;
extern volatile uint32_t g_spo2_window_count;
extern volatile uint32_t g_spo2_correlation_x1000;

void SpO2_Init(void);
void SpO2_ProcessSample(uint32_t red_raw, uint32_t ir_raw);

#endif
