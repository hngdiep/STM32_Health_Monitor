#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sensor_task.h"
#include "i2c1.h"
#include "max30102.h"
#include "mlx90614.h"
#include "bpm.h"
#include "spo2.h"
#include "health_queue.h"

volatile uint32_t g_mlx90614_found = 0;
volatile uint32_t g_i2c_test_count = 0;
volatile int32_t g_mlx_object_centi_c = 0;
volatile int32_t g_mlx_ambient_centi_c = 0;
volatile uint32_t g_mlx_object_ok = 0;
volatile uint32_t g_mlx_ambient_ok = 0;
volatile uint32_t g_mlx_read_errors = 0;

void Sensor_Task(void *argument)
{
    (void)argument;

    TickType_t last_publish = 0;
    TickType_t last_mlx_read = 0;
    int32_t temperature = 0;

    MAX30102_Probe();
    g_mlx90614_found = SensorI2C1_Probe(0x5A);
    MAX30102_Init();

    for (;;)
    {
        uint8_t overflow = 0U;
        uint8_t available = MAX30102_PollAvailable(&overflow);

        if (overflow > 0U)
        {
            BPM_Init();
            SpO2_Init();
        }

        while (available > 0U)
        {
            uint32_t red_raw = 0U;
            uint32_t ir_raw = 0U;

            if (!MAX30102_ReadSample(&red_raw, &ir_raw))
            {
                break;
            }

            if (g_max30102_sample_count > 10U)
            {
                BPM_ProcessIR(ir_raw);
            }

            SpO2_ProcessSample(red_raw, ir_raw);
            --available;
        }

        if ((xTaskGetTickCount() - last_mlx_read) >= pdMS_TO_TICKS(500))
        {
            last_mlx_read = xTaskGetTickCount();

            g_mlx_object_ok = MLX90614_ReadCentiC(0x07, &temperature);

            if (g_mlx_object_ok)
            {
                g_mlx_object_centi_c = temperature;
            }
            else
            {
                ++g_mlx_read_errors;
            }

            g_mlx_ambient_ok = MLX90614_ReadCentiC(0x06, &temperature);

            if (g_mlx_ambient_ok)
            {
                g_mlx_ambient_centi_c = temperature;
            }
            else
            {
                ++g_mlx_read_errors;
            }
        }

        ++g_i2c_test_count;
        TickType_t now = xTaskGetTickCount();
        if ((now - last_publish) >= pdMS_TO_TICKS(1000))
        {
            last_publish = now;

            HealthData_t data = {0};

            data.timestamp_ms =
                (uint32_t)(((uint64_t)now * 1000ULL) / configTICK_RATE_HZ);

            data.bpm_x100 = g_bpm_x100;
            data.spo2_demo_x100 = g_spo2_demo_x100;
            data.spo2_ratio_x10000 = g_spo2_ratio_x10000;

            data.object_temp_centi_c = g_mlx_object_centi_c;
            data.ambient_temp_centi_c = g_mlx_ambient_centi_c;

            data.bpm_valid = (uint8_t)(g_bpm_valid != 0U);
            data.spo2_signal_valid = (uint8_t)(g_spo2_valid != 0U);
            data.object_temp_valid = (uint8_t)(g_mlx_object_ok != 0U);
            data.ambient_temp_valid = (uint8_t)(g_mlx_ambient_ok != 0U);

            HealthQueue_Publish(&data);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
