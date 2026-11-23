#ifndef SD_RTC_H
#define SD_RTC_H

#include <stdbool.h>
#include <stdint.h>

/* Epoch = 2000-01-01 00:00:00; timestamp is seconds since this epoch. */
#define SD_RTC_INVALID_TIMESTAMP UINT32_MAX

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} SD_RTC_DateTime;

bool SD_RTC_Init(void);
bool SD_RTC_IsSet(void);
bool SD_RTC_SetDateTime(const SD_RTC_DateTime *date_time);
bool SD_RTC_GetDateTime(SD_RTC_DateTime *date_time);
uint32_t SD_RTC_Now(void);
bool SD_RTC_ToDateTime(uint32_t timestamp, SD_RTC_DateTime *date_time);

#endif