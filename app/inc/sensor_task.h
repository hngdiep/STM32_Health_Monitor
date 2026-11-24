#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>

extern volatile uint32_t g_mlx90614_found;
extern volatile uint32_t g_i2c_test_count;
extern volatile int32_t g_mlx_object_centi_c;
extern volatile int32_t g_mlx_ambient_centi_c;
extern volatile uint32_t g_mlx_object_ok;
extern volatile uint32_t g_mlx_ambient_ok;
extern volatile uint32_t g_mlx_read_errors;

void Sensor_Task(void *argument);

#endif
