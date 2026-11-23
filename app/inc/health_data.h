#ifndef HEALTH_DATA_H
#define HEALTH_DATA_H
#include <stdint.h>
typedef struct {
    uint16_t bpm;
    uint8_t spo2;
    int16_t temp_x10;  /* e.g. 367 = 36.7 C */
    uint8_t valid;     /* 1 = sample valid */
    uint32_t updated_ms;
} HealthReading;
typedef struct {
    uint16_t bpm_max;
    uint8_t spo2_min;
    int16_t temp_max_x10;
} HealthLimits;
void HealthData_Init(void);
void HealthData_SetMeasurements(uint16_t bpm,uint8_t spo2,int16_t temp_x10,uint8_t valid);
HealthReading HealthData_Get(void);
HealthLimits HealthLimits_Get(void);
void HealthLimits_Set(HealthLimits limits);
#endif
