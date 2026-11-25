
#include "health_data.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>

static HealthReading g_reading;
static HealthLimits g_limits;

/* Khoi tao du lieu va nguong canh bao */
void HealthData_Init(void)
{
    g_reading = (HealthReading){0};

    g_limits.bpm_max = 110;
    g_limits.spo2_min = 94;
    g_limits.temp_max_x10 = 380;
}

/* Cap nhat du lieu gia lap cho TFT */
void HealthData_SetMeasurements(
    uint16_t bpm,
    uint8_t spo2,
    int16_t temp_x10,
    uint8_t valid
)
{
    HealthReading r = {0};

    r.bpm = bpm;
    r.spo2 = spo2;
    r.temp_x10 = temp_x10;

    r.valid = valid ? 1U : 0U;

    r.bpm_valid = r.valid;
    r.spo2_valid = r.valid;
    r.temp_valid = r.valid;

    /* Du lieu do ham nay cung cap la demo */
    r.spo2_demo = 1U;

    r.updated_ms =
        (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;

    taskENTER_CRITICAL();
    g_reading = r;
    taskEXIT_CRITICAL();
}

/* Chuyen du lieu cam bien sang du lieu TFT */
void HealthData_SetFromSensor(const HealthData_t *sensor)
{
    if (sensor == NULL)
    {
        return;
    }

    HealthReading r = {0};

    /* BPM: x100 -> BPM */
    if (sensor->bpm_valid)
    {
        uint32_t bpm =
            sensor->bpm_x100 / 100U +
            ((sensor->bpm_x100 % 100U) >= 50U ? 1U : 0U);

        if (bpm <= UINT16_MAX)
        {
            r.bpm = (uint16_t)bpm;
            r.bpm_valid = 1U;
        }
    }

    /* SpO2: gia tri uoc luong, chua hieu chuan */
    if (sensor->spo2_signal_valid)
    {
        uint32_t spo2 =
            sensor->spo2_demo_x100 / 100U +
            ((sensor->spo2_demo_x100 % 100U) >= 50U ? 1U : 0U);

        if (spo2 <= 100U)
        {
            r.spo2 = (uint8_t)spo2;
            r.spo2_valid = 1U;
            r.spo2_demo = 1U;
        }
    }

    /* Nhiet do vat the: 0.01 C -> 0.1 C */
    if (sensor->object_temp_valid)
    {
        int32_t temp =
            sensor->object_temp_centi_c / 10;

        if (temp >= INT16_MIN && temp <= INT16_MAX)
        {
            r.temp_x10 = (int16_t)temp;
            r.temp_valid = 1U;
        }
    }

    /* Co du lieu neu it nhat mot chi so hop le */
    r.valid = (uint8_t)(
        r.bpm_valid ||
        r.spo2_valid ||
        r.temp_valid
    );

    r.updated_ms =
        (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;

    taskENTER_CRITICAL();
    g_reading = r;
    taskEXIT_CRITICAL();
}

/* Doc du lieu cho TFT va canh bao */
HealthReading HealthData_Get(void)
{
    HealthReading r;

    taskENTER_CRITICAL();
    r = g_reading;
    taskEXIT_CRITICAL();

    uint32_t now_ms =
        (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;

    /* Du lieu qua 5 giay se khong con hop le */
    if ((uint32_t)(now_ms - r.updated_ms) > 5000U)
    {
        r.valid = 0U;
        r.bpm_valid = 0U;
        r.spo2_valid = 0U;
        r.temp_valid = 0U;
        r.spo2_demo = 0U;
    }

    return r;
}

/* Doc nguong canh bao */
HealthLimits HealthLimits_Get(void)
{
    HealthLimits l;

    taskENTER_CRITICAL();
    l = g_limits;
    taskEXIT_CRITICAL();

    return l;
}

/* Cap nhat nguong canh bao */
void HealthLimits_Set(HealthLimits limits)
{
    taskENTER_CRITICAL();
    g_limits = limits;
    taskEXIT_CRITICAL();
}
