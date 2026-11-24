#include "sd_task.h"
#include "sd_rtc.h"
#include "sd_spi.h"
#include "ff.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define SD_LOG_BUFFER_SIZE 512

/* No floating-point printf required (temperature is in centi-degrees). */
static const char csv_header[] =
    "timestamp,bpm,spo2,temperature_c,signal_status\r\n";

typedef struct {
    SD_LogSample data;
    uint32_t timestamp;
} QueuedSample;

QueueHandle_t SD_LogQueue;
static TaskHandle_t logging_task_handle;
static FATFS sd_fs;
static volatile SD_LogStats log_stats = {
    .state = SD_LOG_OFFLINE
};

static const char *signal_text(SD_SignalStatus signal)
{
    switch (signal) {
        case SIGNAL_VALID: return "VALID";
        case SIGNAL_WEAK: return "WEAK";
        case SIGNAL_NO_FINGER: return "NO_FINGER";
        case SIGNAL_SENSOR_ERROR: return "SENSOR_ERROR";
        default: return "UNKNOWN";
    }
}

static bool append_formatted(char *dest, size_t dest_capacity,
                             size_t *length, const QueuedSample *s)
{
    SD_RTC_DateTime dt;
    char timestamp[20];
    if (SD_RTC_ToDateTime(s->timestamp, &dt)) {
        int used = snprintf(timestamp, sizeof(timestamp),
                            "%04u-%02u-%02u %02u:%02u:%02u",
                            (unsigned)dt.year, (unsigned)dt.month,
                            (unsigned)dt.day, (unsigned)dt.hour,
                            (unsigned)dt.minute, (unsigned)dt.second);
        if (used != 19) return false;
    } else {
        (void)snprintf(timestamp, sizeof(timestamp), "RTC_UNSET");
    }

    int32_t temp = s->data.temperature_centi_c;
    const char *sign = (temp < 0) ? "-" : "";
    if (temp < 0) temp = -temp;
    if (*length >= dest_capacity) return false;
    int count = snprintf(dest + *length, dest_capacity - *length,
                         "%s,%u,%u,%s%ld.%02ld,%s\r\n",
                         timestamp,
                         (unsigned)s->data.bpm,
                         (unsigned)s->data.spo2,
                         sign, (long)(temp / 100), (long)(temp % 100),
                         signal_text(s->data.signal));
    if (count < 0 || (size_t)count >= dest_capacity - *length) return false;
    *length += (size_t)count;
    return true;
}

/* Verify exact appended bytes, not only the return status of f_write. */
static bool verify_written_bytes(FIL *file, DWORD begin,
                                 const char *written, UINT length)
{
    if (f_lseek(file, begin) != FR_OK) return false;
    char check[64];
    UINT offset = 0;
    while (offset < length) {
        UINT n = length - offset;
        if (n > sizeof(check)) n = sizeof(check);
        UINT got = 0;
        if (f_read(file, check, n, &got) != FR_OK || got != n) return false;
        if (memcmp(check, written + offset, n) != 0) return false;
        offset += n;
    }
    return true;
}

/* Called only by Logging_Task; f_mount/f_open not shared with other tasks. */
static bool mount_card(void)
{
    FRESULT fr = f_mount(&sd_fs, "0:", 1);
    log_stats.last_fresult = fr;
    if (fr != FR_OK) {
        log_stats.mount_failures++;
        SD_SPI_Invalidate();
        log_stats.state = SD_LOG_OFFLINE;
        return false;
    }
    log_stats.state = SD_LOG_READY;
    return true;
}

/* Write a batch, f_sync it, read it back, and close the file. */
static bool write_group(const QueuedSample *samples, size_t count)
{
    /* 8.3 names are supported even when FatFS long filenames are disabled. */
    unsigned user_id = (unsigned)samples[0].data.user_id;
    if (user_id == 0 || user_id > SD_LOG_MAX_USERS) return false;
    char path[16];
    int name_length = snprintf(path, sizeof(path), "0:/U%03u.CSV", user_id);
    if (name_length <= 0 || (size_t)name_length >= sizeof(path)) return false;
    char csv[SD_LOG_BUFFER_SIZE];
    size_t csv_length = 0;
    for (size_t i = 0; i < count; i++) {
        if (!append_formatted(csv, sizeof(csv), &csv_length, &samples[i])) {
            log_stats.write_failures++;
            log_stats.state = SD_LOG_WRITE_ERROR;
            return false;
        }
    }

    FIL file;
    FRESULT fr = f_open(&file, path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if (fr != FR_OK) goto write_error_no_file;

    DWORD begin = f_size(&file);
    fr = f_lseek(&file, begin);
    if (fr != FR_OK) goto write_error;

    if (begin == 0) {
        UINT header_bytes = 0;
        fr = f_write(&file, csv_header, sizeof(csv_header) - 1, &header_bytes);
        if (fr != FR_OK || header_bytes != sizeof(csv_header) - 1) goto write_error;
        begin = f_tell(&file);
    }

    UINT bytes_written = 0;
    fr = f_write(&file, csv, (UINT)csv_length, &bytes_written);
    if (fr != FR_OK || bytes_written != csv_length) goto write_error;

    fr = f_sync(&file);
    if (fr != FR_OK) goto write_error;
    if (!verify_written_bytes(&file, begin, csv, (UINT)csv_length)) {
        log_stats.verify_failures++;
        log_stats.state = SD_LOG_VERIFY_ERROR;
        (void)f_close(&file);
        return false;
    }

    fr = f_close(&file);
    if (fr != FR_OK) goto write_error_no_file;
    log_stats.samples_written += count;
    log_stats.batches_verified++;
    log_stats.last_fresult = FR_OK;
    log_stats.state = SD_LOG_READY;
    return true;

write_error:
    (void)f_close(&file);
write_error_no_file:
    log_stats.last_fresult = fr;
    log_stats.write_failures++;
    log_stats.state = SD_LOG_WRITE_ERROR;
    return false;
}

/* A batch may contain samples from different users after UI switches.
   Write consecutive same-user runs into their respective files.
   Remove successfully persisted runs if a later run fails, so the caller
   does not retry those already committed. */
static bool write_batch(QueuedSample *samples, size_t *remaining)
{
    size_t done = 0;
    while (done < *remaining) {
        uint8_t user_id = samples[done].data.user_id;
        size_t group_end = done + 1;
        while (group_end < *remaining &&
               samples[group_end].data.user_id == user_id) {
            group_end++;
        }
        if (!write_group(&samples[done], group_end - done)) {
            if (done > 0) {
                memmove(samples, samples + done,
                        (*remaining - done) * sizeof(samples[0]));
                *remaining -= done;
            }
            return false;
        }
        done = group_end;
    }
    *remaining = 0;
    return true;
}

void SD_Logging_GetStats(SD_LogStats *stats)
{
    if (stats == 0) return;
    /* Approximate live telemetry; sampled without a lock. */
    stats->state = log_stats.state;
    stats->samples_written = log_stats.samples_written;
    stats->batches_verified = log_stats.batches_verified;
    stats->samples_dropped = log_stats.samples_dropped;
    stats->mount_failures = log_stats.mount_failures;
    stats->write_failures = log_stats.write_failures;
    stats->verify_failures = log_stats.verify_failures;
    stats->last_fresult = log_stats.last_fresult;
}

bool SD_Logging_Init(void)
{
    if (SD_LogQueue != 0) return true;
    SD_LogQueue = xQueueCreate(SD_LOG_QUEUE_LENGTH, sizeof(QueuedSample));
    if (SD_LogQueue == 0) return false;

    BaseType_t ok = xTaskCreate(Logging_Task, "Logging_Task", 512,
                                0, tskIDLE_PRIORITY + 2, &logging_task_handle);
    if (ok != pdPASS) {
        vQueueDelete(SD_LogQueue);
        SD_LogQueue = 0;
        return false;
    }
    return true;
}

bool SD_Logging_Publish(const SD_LogSample *sample)
{
    if (sample == 0 || SD_LogQueue == 0) return false;
    if (sample->user_id == 0 || sample->user_id > SD_LOG_MAX_USERS) return false;
    QueuedSample packet;
    packet.data = *sample;
    packet.timestamp = SD_RTC_Now();
    if (xQueueSend(SD_LogQueue, &packet, 0) == pdPASS) return true;
    log_stats.samples_dropped++;
    return false;
}

void Logging_Task(void *argument)
{
    (void)argument;
    QueuedSample batch[SD_LOG_BATCH_SIZE];
    size_t pending = 0;
    TickType_t first_pending_tick = 0;
    bool mounted = false;

    for (;;) {
        if (!mounted) {
            if (!mount_card()) {
                vTaskDelay(pdMS_TO_TICKS(SD_LOG_RETRY_MS));
                continue;
            }
            mounted = true;
        }

        if (pending < SD_LOG_BATCH_SIZE) {
            QueuedSample message;
            if (xQueueReceive(SD_LogQueue, &message,
                              pdMS_TO_TICKS(250)) == pdPASS) {
                if (pending == 0) first_pending_tick = xTaskGetTickCount();
                batch[pending++] = message;
            }
        }

        if (pending > 0 &&
            (pending == SD_LOG_BATCH_SIZE ||
             (xTaskGetTickCount() - first_pending_tick) >= pdMS_TO_TICKS(SD_LOG_FLUSH_MS))) {
            if (write_batch(batch, &pending)) {
                /* All samples persisted into their user-specific files. */
            } else {
                /* Keep this batch in RAM for retry; sensor queue continues
                   receiving up to its fixed capacity. No other task blocks. */
                (void)f_mount(0, "0:", 0);
                SD_SPI_Invalidate();
                mounted = false;
                vTaskDelay(pdMS_TO_TICKS(SD_LOG_RETRY_MS));
            }
        }
    }
}