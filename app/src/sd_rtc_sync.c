#include <stddef.h>
#include "sd_rtc_sync.h"
#include "sd_rtc.h"
#include "FreeRTOS.h"
#include "task.h"

/* Non-static symbols intentionally visible to arm-none-eabi-nm / ST-Link. */
volatile uint32_t g_sd_sync_seconds = 0;
volatile uint32_t g_sd_sync_request = 0;
volatile uint32_t g_sd_sync_result = 0;

static void SD_RTC_Sync_Task(void *argument)
{
    (void)argument;

    for (;;) {
        if (g_sd_sync_request == 1) {
            const uint32_t seconds = g_sd_sync_seconds;
            SD_RTC_DateTime time;

            /* Clear the request before reporting completion. */
            g_sd_sync_request = 0;
            if (SD_RTC_ToDateTime(seconds, &time) &&
                SD_RTC_SetDateTime(&time)) {
                g_sd_sync_result = 1;
            } else {
                g_sd_sync_result = 2;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool SD_RTC_Sync_Start(void)
{
    /* A small independent task, also present in the normal team firmware. */
    return xTaskCreate(SD_RTC_Sync_Task, "RTC_SYNC", 160,
                       NULL, tskIDLE_PRIORITY + 2, NULL) == pdPASS;
}
