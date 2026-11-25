
#ifndef HEALTH_DATA_H
#define HEALTH_DATA_H

#include <stdint.h>

/* Du lieu goc tu cam bien - GIU NGUYEN */
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

/* Du lieu phuc vu TFT */
typedef struct
{
    uint16_t bpm;
    uint8_t spo2;
    int16_t temp_x10;

    uint8_t valid;
    uint8_t bpm_valid;
    uint8_t spo2_valid;
    uint8_t temp_valid;

    uint8_t spo2_demo;

    uint32_t updated_ms;

} HealthReading;

/* Nguong canh bao */
typedef struct
{
    uint16_t bpm_max;
    uint8_t spo2_min;
    int16_t temp_max_x10;

} HealthLimits;

/* Quan ly du lieu giao dien */
void HealthData_Init(void);

void HealthData_SetMeasurements(
    uint16_t bpm,
    uint8_t spo2,
    int16_t temp_x10,
    uint8_t valid
);

/* Chuyen du lieu cam bien sang du lieu TFT */
void HealthData_SetFromSensor(const HealthData_t *sensor);

HealthReading HealthData_Get(void);

/* Quan ly nguong canh bao */
HealthLimits HealthLimits_Get(void);
void HealthLimits_Set(HealthLimits limits);

#endif
