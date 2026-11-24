
#ifndef HEALTH_QUEUE_H
#define HEALTH_QUEUE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "health_data.h"

extern QueueHandle_t g_health_ui_queue;
extern QueueHandle_t g_health_warning_queue;
extern QueueHandle_t g_health_logger_queue;

extern volatile uint32_t g_health_queue_ready;
extern volatile uint32_t g_health_publish_count;
extern volatile uint32_t g_health_logger_drop_count;

BaseType_t HealthQueue_Init(void);
void HealthQueue_Publish(const HealthData_t *data);

#endif
