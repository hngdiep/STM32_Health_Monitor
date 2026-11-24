#ifndef MONITOR_TASK_H
#define MONITOR_TASK_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

extern volatile uint32_t g_monitor_counter;
extern volatile uint32_t g_heap_free_bytes;
extern volatile uint32_t g_heap_min_free_bytes;
extern volatile uint32_t g_led_stack_free_words;
extern volatile uint32_t g_mon_stack_free_words;
extern volatile uint32_t g_sensor_stack_free_words;

void MonitorTask_SetHandles(TaskHandle_t led, TaskHandle_t monitor, TaskHandle_t sensor);
void Monitor_Task(void *argument);

#endif
