#include <stdint.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"

volatile uint32_t g_monitor_counter = 0;

static void LED_Task(void *argument)
{
    (void)argument;

    for (;;)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(500));
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void Monitor_Task(void *argument)
{
    (void)argument;

    for (;;)
    {
        ++g_monitor_counter;
        vTaskDelay(pdMS_TO_TICKS(250));
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
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    if (xTaskCreate(LED_Task, "LED", 192, NULL, 2, NULL) != pdPASS)
    {
        for (;;) {}
    }

    if (xTaskCreate(Monitor_Task, "MON", 192, NULL, 1, NULL) != pdPASS)
    {
        for (;;) {}
    }

    vTaskStartScheduler();
    for (;;) {}
}
