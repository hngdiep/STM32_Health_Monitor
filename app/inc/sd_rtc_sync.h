#ifndef SD_RTC_SYNC_H
#define SD_RTC_SYNC_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ST-Link live RTC sync protocol.
 * Host sends local wall-clock seconds since 2000-01-01 (NOT Unix epoch).
 * The FreeRTOS task applies the request to the existing SD_RTC driver.
 * Available in BOTH normal and SD_TEST builds.
 */
extern volatile uint32_t g_sd_sync_seconds;
extern volatile uint32_t g_sd_sync_request;
extern volatile uint32_t g_sd_sync_result;

/* Result: 0 waiting/not synced this boot, 1 success, 2 invalid/rejected. */
/* Must call SD_RTC_Init() before calling this. Call before scheduler starts. */
bool SD_RTC_Sync_Start(void);

#endif
