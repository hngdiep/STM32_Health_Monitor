#ifndef SD_TASK_H
#define SD_TASK_H

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

#define SD_LOG_BATCH_SIZE       4
#define SD_LOG_QUEUE_LENGTH    16
#define SD_LOG_FLUSH_MS        5000
#define SD_LOG_RETRY_MS        2000
#define SD_LOG_MAX_USERS       10  /* IDs 1..10; 0 means no selection */

/* The signal-quality result comes from the MAX30102 processing task. */
typedef enum {
    SIGNAL_VALID = 0,
    SIGNAL_WEAK = 1,
    SIGNAL_NO_FINGER = 2,
    SIGNAL_SENSOR_ERROR = 3
} SD_SignalStatus;


typedef struct {
    uint16_t bpm;
    uint8_t spo2;
    int16_t temperature_centi_c;

    SD_SignalStatus signal;
    uint8_t user_id;

    /* 1 = co du lieu hop le, 0 = khong co */
    uint8_t bpm_valid;
    uint8_t spo2_valid;
    uint8_t temp_valid;
} SD_LogSample;


typedef enum {
    SD_LOG_OFFLINE = 0,
    SD_LOG_READY = 1,
    SD_LOG_WRITE_ERROR = 2,
    SD_LOG_VERIFY_ERROR = 3
} SD_LogState;

typedef struct {
    SD_LogState state;
    uint32_t samples_written;
    uint32_t batches_verified;
    uint32_t samples_dropped;
    uint32_t mount_failures;
    uint32_t write_failures;
    uint32_t verify_failures;
    int last_fresult;
} SD_LogStats;

extern QueueHandle_t SD_LogQueue;

bool SD_Logging_Init(void);
/* Nonblocking; call from Sensor_Task (not from an ISR). Timestamp is added
   at publish time via SD_RTC_Now(), not when the SD write occurs. */
/* Rejects user_id = 0 or > SD_LOG_MAX_USERS. The user ID is copied into the queue. */
bool SD_Logging_Publish(const SD_LogSample *sample);
void Logging_Task(void *argument);
void SD_Logging_GetStats(SD_LogStats *stats);

#endif