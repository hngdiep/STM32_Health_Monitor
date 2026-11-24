
#include "health_queue.h"

QueueHandle_t g_health_ui_queue = NULL;
QueueHandle_t g_health_warning_queue = NULL;
QueueHandle_t g_health_logger_queue = NULL;

volatile uint32_t g_health_queue_ready = 0;
volatile uint32_t g_health_publish_count = 0;
volatile uint32_t g_health_logger_drop_count = 0;

BaseType_t HealthQueue_Init(void)
{
    g_health_ui_queue = xQueueCreate(1U, sizeof(HealthData_t));
    g_health_warning_queue = xQueueCreate(1U, sizeof(HealthData_t));
    g_health_logger_queue = xQueueCreate(8U, sizeof(HealthData_t));

    if (g_health_ui_queue == NULL ||
        g_health_warning_queue == NULL ||
        g_health_logger_queue == NULL)
    {
        if (g_health_ui_queue != NULL)
            vQueueDelete(g_health_ui_queue);

        if (g_health_warning_queue != NULL)
            vQueueDelete(g_health_warning_queue);

        if (g_health_logger_queue != NULL)
            vQueueDelete(g_health_logger_queue);

        g_health_ui_queue = NULL;
        g_health_warning_queue = NULL;
        g_health_logger_queue = NULL;

        g_health_queue_ready = 0;
        return pdFAIL;
    }

    g_health_queue_ready = 1;
    return pdPASS;
}

void HealthQueue_Publish(const HealthData_t *data)
{
    if (data == NULL || g_health_queue_ready == 0U)
        return;

    xQueueOverwrite(g_health_ui_queue, data);
    xQueueOverwrite(g_health_warning_queue, data);

    if (xQueueSend(g_health_logger_queue, data, 0) != pdPASS)
    {
        ++g_health_logger_drop_count;
    }

    ++g_health_publish_count;
}
