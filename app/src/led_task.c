#include "stm32f10x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led_task.h"

void LED_Task(void *argument)
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
