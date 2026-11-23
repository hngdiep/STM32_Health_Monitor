#include "health_data.h"
#include "FreeRTOS.h"
#include "task.h"
static HealthReading g_reading;
static HealthLimits g_limits;
void HealthData_Init(void)
{
    g_reading.bpm=0;g_reading.spo2=0;g_reading.temp_x10=0;
    g_reading.valid=0;g_reading.updated_ms=0;
    g_limits.bpm_max=110;g_limits.spo2_min=94;g_limits.temp_max_x10=380;
}
void HealthData_SetMeasurements(uint16_t bpm,uint8_t spo2,int16_t temp_x10,uint8_t valid)
{
    taskENTER_CRITICAL();
    g_reading.bpm=bpm;g_reading.spo2=spo2;g_reading.temp_x10=temp_x10;
    g_reading.valid=valid ? 1 : 0;
    g_reading.updated_ms=xTaskGetTickCount()*portTICK_PERIOD_MS;
    taskEXIT_CRITICAL();
}
HealthReading HealthData_Get(void)
{
    HealthReading r;
    taskENTER_CRITICAL();r=g_reading;taskEXIT_CRITICAL();
    if(r.valid && (uint32_t)(xTaskGetTickCount()*portTICK_PERIOD_MS-r.updated_ms)>5000U) r.valid=0;
    return r;
}
HealthLimits HealthLimits_Get(void)
{
    HealthLimits l;
    taskENTER_CRITICAL();l=g_limits;taskEXIT_CRITICAL();return l;
}
void HealthLimits_Set(HealthLimits l)
{
    taskENTER_CRITICAL();g_limits=l;taskEXIT_CRITICAL();
}
