
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "i2c1.h"
#include "bpm.h"
#include "spo2.h"

#include "health_data.h"
#include "health_queue.h"
#include "health_bridge.h"

#include "sensor_task.h"
#include "monitor_task.h"

#include "health_ui.h"
#include "health_controls.h"

#include "sd_rtc.h"
#include "sd_rtc_sync.h"

/* Task handles */
static TaskHandle_t s_mon_handle = NULL;
static TaskHandle_t s_sensor_handle = NULL;

/*
 * FreeRTOS malloc failed hook.
 * Duoc goi khi cap phat Heap that bai.
 */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}

/*
 * FreeRTOS stack overflow hook.
 * Duoc goi khi phat hien Task tran Stack.
 */
void vApplicationStackOverflowHook(
    TaskHandle_t task,
    char *taskName
)
{
    (void)task;
    (void)taskName;

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
    }
}

int main(void)
{
    /*
     * 1. Khoi tao cac cam bien.
     */
    SensorI2C1_Init();
    BPM_Init();
    SpO2_Init();

    /*
     * 2. Khoi tao du lieu dung chung
     * cho TFT, nut nhan va canh bao.
     */
    HealthData_Init();

    /*
     * 3. Tao cac Health Queue.
     */
    if (HealthQueue_Init() != pdPASS)
    {
        goto fail;
    }

    /*
     * 4. Tao Task giam sat he thong.
     */
    if (xTaskCreate(
            Monitor_Task,
            "MON",
            192,
            NULL,
            1,
            &s_mon_handle
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * 5. Tao Task doc cam bien.
     */
    if (xTaskCreate(
            Sensor_Task,
            "SENSOR",
            256,
            NULL,
            1,
            &s_sensor_handle
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * 6. Tao Health Bridge Task.
     *
     * Nhan du lieu cam bien tu Queue,
     * sau do cap nhat HealthData.
     */
    if (xTaskCreate(
            Health_Bridge_Task,
            "BRIDGE",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * 7. Tao Task hien thi TFT.
     */
    if (xTaskCreate(
            Health_UI_Task,
            "UI",
            384,
            NULL,
            1,
            NULL
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * 8. Tao Task xu ly nut nhan.
     *
     * SET  : PA0
     * UP   : PA1
     * DOWN : PA2
     */
    if (xTaskCreate(
            Health_Buttons_Task,
            "BUTTON",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * 9. Tao Task canh bao.
     *
     * LED    : PB8
     * Buzzer : PB9
     */
    if (xTaskCreate(
            Health_Warning_Task,
            "WARNING",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        goto fail;
    }

    /*
     * Khong tao LED_Task cu.
     *
     * Vi LED_Task cu su dung PA0,
     * trung voi nut SET.
     */
    MonitorTask_SetHandles(
        NULL,
        s_mon_handle,
        s_sensor_handle
    );

    /*
     * 10. Khoi tao RTC cua nhom.
     */
    if (!SD_RTC_Init())
    {
        goto fail;
    }

    if (!SD_RTC_Sync_Start())
    {
        goto fail;
    }

    /*
     * 11. Khoi dong FreeRTOS Scheduler.
     */
    vTaskStartScheduler();

fail:
    /*
     * Neu khoi tao that bai
     * hoac Scheduler khong khoi dong,
     * dung chuong trinh tai day.
     */
    for (;;)
    {
    }
}
