
#include <stdint.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2c1.h"
#include "bpm.h"
#include "spo2.h"
#include "health_queue.h"
#include "sensor_task.h"
#include "led_task.h"
#include "monitor_task.h"

static TaskHandle_t s_led_handle = NULL;
static TaskHandle_t s_mon_handle = NULL;
static TaskHandle_t s_sensor_handle = NULL;

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

    MonitorTask_SetHandles(s_led_handle, s_mon_handle, s_sensor_handle);

    vTaskStartScheduler();

    for (;;) {}
}
