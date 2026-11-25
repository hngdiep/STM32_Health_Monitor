
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "health_data.h"
#include "health_queue.h"

void Health_Bridge_Task(void *argument)
{
    HealthData_t sensor_data;

    (void)argument;

    for (;;)
    {
        if (xQueueReceive(
                g_health_ui_queue,
                &sensor_data,
                portMAX_DELAY
            ) == pdPASS)
        {
            HealthData_SetFromSensor(&sensor_data);
        }
    }
}
