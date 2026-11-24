#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>

extern volatile uint32_t g_max30102_sample_count;

int MAX30102_Probe(void);
void MAX30102_Init(void);
uint8_t MAX30102_PollAvailable(uint8_t *overflow_out);
int MAX30102_ReadSample(uint32_t *red_raw, uint32_t *ir_raw);

#endif
