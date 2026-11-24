
#ifndef HEALTH_DATA_H
#define HEALTH_DATA_H

#include <stdint.h>

typedef struct
{
    uint32_t timestamp_ms;

    uint32_t bpm_x100;
    uint32_t spo2_demo_x100;
    uint32_t spo2_ratio_x10000;

    int32_t object_temp_centi_c;
    int32_t ambient_temp_centi_c;

    uint8_t bpm_valid;
    uint8_t spo2_signal_valid;
    uint8_t object_temp_valid;
    uint8_t ambient_temp_valid;

} HealthData_t;

#endif
