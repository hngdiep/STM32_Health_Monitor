#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "health_data.h"
#include "health_ui.h"
#include "health_controls.h"

/* 1 = demo without sensors; change to 0 when real sensor task calls HealthData_SetMeasurements(). */
#define HEALTH_DEMO_MODE 1

static void Heartbeat_Task(void *arg)
{
    GPIO_InitTypeDef g;
    (void)arg;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    GPIO_StructInit(&g);
    g.GPIO_Pin=GPIO_Pin_13;
    g.GPIO_Mode=GPIO_Mode_Out_PP;
    g.GPIO_Speed=GPIO_Speed_2MHz;
    GPIO_Init(GPIOC,&g);
    for(;;) {
        GPIO_ResetBits(GPIOC,GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(100));
        GPIO_SetBits(GPIOC,GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#if HEALTH_DEMO_MODE
static void Health_Demo_Task(void *arg)
{
    uint32_t t=0;
    (void)arg;
    for(;;) {
        /* 8 seconds healthy then 4 seconds alert; values are NOT measurements. */
        if((t%12)<8) HealthData_SetMeasurements(78,98,367,1);
        else HealthData_SetMeasurements(122,91,388,1);
        t++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

int main(void)
{
    HealthData_Init();
    if(xTaskCreate(Heartbeat_Task,"LED",192,NULL,2,NULL)!=pdPASS) goto fail;
    if(xTaskCreate(Health_UI_Task,"UI",512,NULL,1,NULL)!=pdPASS) goto fail;
    if(xTaskCreate(Health_Buttons_Task,"Buttons",256,NULL,2,NULL)!=pdPASS) goto fail;
    if(xTaskCreate(Health_Warning_Task,"Warning",256,NULL,2,NULL)!=pdPASS) goto fail;
#if HEALTH_DEMO_MODE
    if(xTaskCreate(Health_Demo_Task,"Demo",192,NULL,1,NULL)!=pdPASS) goto fail;
#endif
    vTaskStartScheduler();
fail:
    for(;;) {}
}
/* Keep only one definition of these hooks in the whole project. */
void vApplicationMallocFailedHook(void) {taskDISABLE_INTERRUPTS();for(;;) {}}
void vApplicationStackOverflowHook(TaskHandle_t task,char *name)
{(void)task;(void)name;taskDISABLE_INTERRUPTS();for(;;) {}}
