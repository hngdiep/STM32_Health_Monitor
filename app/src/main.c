
#include <stdint.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2c1.h"
#include "bpm.h"
#include "mlx90614.h"
#include "spo2.h"
#include "health_queue.h"



volatile uint32_t g_monitor_counter = 0;
volatile uint32_t g_max30102_found = 0;
volatile uint32_t g_mlx90614_found = 0;
volatile uint32_t g_i2c_test_count = 0;

volatile uint32_t g_max30102_part_id = 0xFFFFFFFFU;
volatile uint32_t g_max30102_read_ok = 0;
volatile uint32_t g_max30102_id_ok = 0;
volatile uint32_t g_max30102_write_ok = 0;
volatile uint32_t g_max30102_id_attempts = 0;
volatile uint32_t g_max30102_id_zero_count = 0;
volatile uint32_t g_max30102_id_i2c_errors = 0;
volatile uint32_t g_max30102_led1_pa = 0xFFFFFFFFU;

volatile uint32_t g_max30102_config_ok = 0;
volatile uint32_t g_max30102_mode_config = 0xFFFFFFFFU;
volatile uint32_t g_max30102_spo2_config = 0xFFFFFFFFU;
volatile uint32_t g_max30102_fifo_wr_ptr = 0;
volatile uint32_t g_max30102_fifo_rd_ptr = 0;
volatile uint32_t g_max30102_fifo_move_count = 0;

volatile uint32_t g_max30102_red_raw = 0;
volatile uint32_t g_max30102_ir_raw = 0;
volatile uint32_t g_max30102_fifo_read_ok = 0;
volatile uint32_t g_max30102_sample_count = 0;
volatile uint32_t g_max30102_fifo_errors = 0;

volatile uint32_t g_max30102_ovf_last = 0;
volatile uint32_t g_max30102_ovf_max = 0;
volatile uint32_t g_max30102_ovf_nonzero_polls = 0;
volatile uint32_t g_max30102_ovf_read_errors = 0;

volatile int32_t g_mlx_object_centi_c = 0;
volatile int32_t g_mlx_ambient_centi_c = 0;
volatile uint32_t g_mlx_object_ok = 0;
volatile uint32_t g_mlx_ambient_ok = 0;
volatile uint32_t g_mlx_read_errors = 0;

volatile uint32_t g_heap_free_bytes = 0;
volatile uint32_t g_heap_min_free_bytes = 0;

volatile uint32_t g_led_stack_free_words = 0;
volatile uint32_t g_mon_stack_free_words = 0;
volatile uint32_t g_sensor_stack_free_words = 0;

static TaskHandle_t s_led_handle = NULL;
static TaskHandle_t s_mon_handle = NULL;
static TaskHandle_t s_sensor_handle = NULL;


static void LED_Task(void *argument)
{
    (void)argument;

    for (;;)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
        vTaskDelay(pdMS_TO_TICKS(500));

        GPIO_ResetBits(GPIOA, GPIO_Pin_0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}




static void Monitor_Task(void *argument)
{
    (void)argument;

    for (;;)
    {
        ++g_monitor_counter;

        g_heap_free_bytes = (uint32_t)xPortGetFreeHeapSize();

        g_heap_min_free_bytes =
            (uint32_t)xPortGetMinimumEverFreeHeapSize();

        if (s_led_handle != NULL)
        {
            g_led_stack_free_words =
                (uint32_t)uxTaskGetStackHighWaterMark(s_led_handle);
        }

        if (s_mon_handle != NULL)
        {
            g_mon_stack_free_words =
                (uint32_t)uxTaskGetStackHighWaterMark(s_mon_handle);
        }

        if (s_sensor_handle != NULL)
        {
            g_sensor_stack_free_words =
                (uint32_t)uxTaskGetStackHighWaterMark(s_sensor_handle);
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}




static void Sensor_Task(void *argument)
{
    (void)argument;

    TickType_t last_publish = 0;
    uint8_t part_id = 0;
    uint8_t value = 0;
    uint8_t mode = 0;
    uint8_t spo2_cfg = 0;
    uint8_t wr = 0;
    uint8_t rd = 0;
    uint8_t previous_wr = 0;
    uint8_t first_read = 1;
    int ok = 0;
    TickType_t last_mlx_read = 0;
    int32_t temperature = 0;
    uint8_t fifo[6];
    uint8_t available = 0;
    uint8_t ovf = 0;

    uint8_t status1 = 0;
    uint8_t status_ok = 0;

    g_max30102_found = SensorI2C1_Probe(0x57);
    g_mlx90614_found = SensorI2C1_Probe(0x5A);

    if (g_max30102_found)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        for (uint32_t i = 0; i < 10U; ++i)
        {
            part_id = 0xA5U;
            ++g_max30102_id_attempts;

            if (SensorI2C1_ReadRegister8(0x57, 0xFF, &part_id))
            {
                g_max30102_read_ok = 1U;
                g_max30102_part_id = part_id;

                if (part_id == 0x15U)
                {
                    g_max30102_id_ok = 1U;
                    break;
                }

                if (part_id == 0U)
                {
                    ++g_max30102_id_zero_count;
                }
            }
            else
            {
                ++g_max30102_id_i2c_errors;
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    if (g_max30102_id_ok)
    {
        ok = SensorI2C1_WriteRegister8(0x57, 0x09, 0x40);

        if (ok)
        {
            ok = 0;

            for (uint32_t i = 0; i < 20; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(2));

                if (SensorI2C1_ReadRegister8(0x57, 0x09, &value) &&
                    (value & 0x40U) == 0)
                {
                    ok = 1;
                    break;
                }
            }
        }

        if (ok)
        {
            ok =
                SensorI2C1_WriteRegister8(0x57, 0x08, 0x10) &&
                SensorI2C1_WriteRegister8(0x57, 0x04, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x05, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x06, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x0A, 0x27) &&
                SensorI2C1_WriteRegister8(0x57, 0x0C, 0x24) &&
                SensorI2C1_WriteRegister8(0x57, 0x0D, 0x24) &&
                SensorI2C1_WriteRegister8(0x57, 0x09, 0x03);
        }

        if (ok)
        {
            ok =
                SensorI2C1_ReadRegister8(0x57, 0x09, &mode) &&
                SensorI2C1_ReadRegister8(0x57, 0x0A, &spo2_cfg) &&
                SensorI2C1_ReadRegister8(0x57, 0x0C, &value);

            if (ok)
            {
                g_max30102_mode_config = mode;
                g_max30102_spo2_config = spo2_cfg;
                g_max30102_led1_pa = value;

                ok = (mode == 0x03 &&
                      spo2_cfg == 0x27 &&
                      value == 0x24);
            }
        }

        g_max30102_write_ok = ok;
        g_max30102_config_ok = ok;
    }

    for (;;)
    {

        if (g_max30102_config_ok)
        {

            status1 = 0U;
            status_ok = (uint8_t)SensorI2C1_ReadRegister8(0x57, 0x00, &status1);

            ovf = 0U;
            if (SensorI2C1_ReadRegister8(0x57, 0x05, &ovf))
            {
                uint32_t overflow = (uint32_t)(ovf & 0x1FU);

                g_max30102_ovf_last = overflow;

                if (overflow > g_max30102_ovf_max)
                {
                    g_max30102_ovf_max = overflow;
                }


                if (overflow > 0U)
                {
                    ++g_max30102_ovf_nonzero_polls;
                    BPM_Init();
                    SpO2_Init();
                }
            }

            else
            {
                ++g_max30102_ovf_read_errors;
            }

            if (SensorI2C1_ReadRegister8(0x57, 0x04, &wr) &&
                SensorI2C1_ReadRegister8(0x57, 0x06, &rd))

            {
                wr &= 0x1FU;
                rd &= 0x1FU;

                g_max30102_fifo_wr_ptr = wr;
                g_max30102_fifo_rd_ptr = rd;

                if (!first_read && wr != previous_wr)
                {
                    ++g_max30102_fifo_move_count;
                }

                previous_wr = wr;
                first_read = 0;


                available = (uint8_t)((wr - rd) & 0x1FU);

                if ((available == 0U) &&
                    (((ovf & 0x1FU) != 0U) ||
                    ((status_ok != 0U) && ((status1 & 0x80U) != 0U))))
                {
                    available = 32U;
                }


                while (available > 0U)
                {
                    if (!SensorI2C1_ReadBurst(0x57, 0x07, fifo, 6))
                    {
                        g_max30102_fifo_read_ok = 0;
                        ++g_max30102_fifo_errors;
                        break;
                    }

                    g_max30102_red_raw =
                        ((((uint32_t)fifo[0] << 16) |
                          ((uint32_t)fifo[1] << 8) |
                          (uint32_t)fifo[2]) & 0x3FFFFU);

                    g_max30102_ir_raw =
                        ((((uint32_t)fifo[3] << 16) |
                          ((uint32_t)fifo[4] << 8) |
                          (uint32_t)fifo[5]) & 0x3FFFFU);


                    g_max30102_fifo_read_ok = 1;
                    ++g_max30102_sample_count;

                    if (g_max30102_sample_count > 10U)
                    {
                        BPM_ProcessIR(g_max30102_ir_raw);
                    }

                    SpO2_ProcessSample(g_max30102_red_raw, g_max30102_ir_raw);

                    --available;
                }
            }
            else
            {
                g_max30102_fifo_read_ok = 0;
                ++g_max30102_fifo_errors;
            }
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

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *taskName)
{
    (void)task;
    (void)taskName;

    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);

    SensorI2C1_Init();
    BPM_Init();
    SpO2_Init();

    if (HealthQueue_Init() != pdPASS)
    {
        for (;;)
        {
        }
    }
    if (xTaskCreate(LED_Task, "LED", 192, NULL, 2, &s_led_handle) != pdPASS)
    {
        for (;;) {}
    }
    if (xTaskCreate(Monitor_Task, "MON", 192, NULL, 1, &s_mon_handle) != pdPASS)
    {
        for (;;) {}
    }
    if (xTaskCreate(Sensor_Task, "SENSOR", 256, NULL, 1, &s_sensor_handle) != pdPASS)
    {
        for (;;) {}
    }

    vTaskStartScheduler();

    for (;;) {}
}
