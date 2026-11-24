#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "monitor_task.h"

volatile uint32_t g_monitor_counter = 0;
volatile uint32_t g_heap_free_bytes = 0;
volatile uint32_t g_heap_min_free_bytes = 0;
volatile uint32_t g_led_stack_free_words = 0;
volatile uint32_t g_mon_stack_free_words = 0;
volatile uint32_t g_sensor_stack_free_words = 0;

static TaskHandle_t s_led_handle = NULL;
static TaskHandle_t s_mon_handle = NULL;
static TaskHandle_t s_sensor_handle = NULL;

void MonitorTask_SetHandles(TaskHandle_t led, TaskHandle_t monitor, TaskHandle_t sensor)
{
    s_led_handle = led;
    s_mon_handle = monitor;
    s_sensor_handle = sensor;
}

void Monitor_Task(void *argument)
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
