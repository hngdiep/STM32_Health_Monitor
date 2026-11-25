
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
#include "sd_task.h"
#include "sd_logger_bridge.h"

static TaskHandle_t s_mon_handle = NULL;
static TaskHandle_t s_sensor_handle = NULL;

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(
    TaskHandle_t task,
    char *taskName
)
{
    (void)task;
    (void)taskName;

    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main(void)
{
    /* Khoi tao cam bien */
    SensorI2C1_Init();
    BPM_Init();
    SpO2_Init();

    /* Khoi tao du lieu TFT */
    HealthData_Init();

    /* Khoi tao Queue */
    if (HealthQueue_Init() != pdPASS)
    {
        for (;;) {}
    }

    /* Task giam sat */
    if (xTaskCreate(
            Monitor_Task,
            "MON",
            192,
            NULL,
            1,
            &s_mon_handle
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Task cam bien */
    if (xTaskCreate(
            Sensor_Task,
            "SENSOR",
            256,
            NULL,
            1,
            &s_sensor_handle
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Task chuyen doi du lieu */
    if (xTaskCreate(
            Health_Bridge_Task,
            "BRIDGE",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Task TFT */
    if (xTaskCreate(
            Health_UI_Task,
            "UI",
            384,
            NULL,
            1,
            NULL
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Task nut nhan */
    if (xTaskCreate(
            Health_Buttons_Task,
            "BUTTON",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Task canh bao */
    if (xTaskCreate(
            Health_Warning_Task,
            "WARNING",
            192,
            NULL,
            2,
            NULL
        ) != pdPASS)
    {
        for (;;) {}
    }


    /* Theo doi Heap va Stack */
    MonitorTask_SetHandles(
        NULL,
        s_mon_handle,
        s_sensor_handle
    );

    /* Khoi tao RTC */
    if (!SD_RTC_Init())
    {
        for (;;) {}
    }

    /* Tao Task dong bo thoi gian RTC */
    if (!SD_RTC_Sync_Start())
    {
        for (;;) {}
    }


    /* Khoi tao SD Logger va tao Logging_Task */
    if (!SD_Logging_Init())
    {
        for (;;) {}
    }

    /* Task chuyen du lieu cam bien sang SD Logger */
    if (xTaskCreate(
            SD_Logger_Bridge_Task,
            "SD_BRIDGE",
            192,
            NULL,
            1,
            NULL
        ) != pdPASS)
    {
        for (;;) {}
    }

    /* Khoi dong FreeRTOS */
    vTaskStartScheduler();

    /* Chi chay den day neu Scheduler khong khoi dong */
    for (;;) {}
}
