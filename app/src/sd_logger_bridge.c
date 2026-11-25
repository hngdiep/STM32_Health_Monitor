
#include "sd_logger_bridge.h"

#include "health_queue.h"
#include "sd_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdint.h>

/* 0 = chua chon nguoi dung, khong ghi SD */
static volatile uint8_t g_selected_user = 0;

/* So mau bo qua khi chua chon nguoi dung */
volatile uint32_t g_sd_bridge_skipped = 0;

/* So lan gui du lieu sang SD Logger that bai */
volatile uint32_t g_sd_bridge_failed = 0;

/* Thiet lap nguoi dung: 0 = tat ghi, 1..10 = user */
void SD_Logger_SetUser(uint8_t user_id)
{
    if (user_id <= SD_LOG_MAX_USERS)
    {
        g_selected_user = user_id;
    }
}

/* Lay ID nguoi dung hien tai */
uint8_t SD_Logger_GetUser(void)
{
    return g_selected_user;
}

/* Task chuyen du lieu cam bien sang SD Logger */
void SD_Logger_Bridge_Task(void *argument)
{
    HealthData_t data;

    (void)argument;

    for (;;)
    {
        /* Cho du lieu moi tu Sensor_Task */
        if (xQueueReceive(
                g_health_logger_queue,
                &data,
                portMAX_DELAY
            ) != pdPASS)
        {
            continue;
        }

        /* Kiem tra nguoi dung dang duoc chon */
        uint8_t user = SD_Logger_GetUser();

        if (user == 0U)
        {
            ++g_sd_bridge_skipped;
            continue;
        }

        /* Khoi tao mau du lieu rong */
        SD_LogSample sample = {0};

        sample.user_id = user;

        /* BPM: chuyen x100 sang BPM */
        if (data.bpm_valid)
        {
            uint32_t bpm =
                data.bpm_x100 / 100U;

            if (bpm <= UINT16_MAX)
            {
                sample.bpm = (uint16_t)bpm;
                sample.bpm_valid = 1U;
            }
        }

        /*
         * SpO2 hien tai la gia tri uoc luong DEMO,
         * chua duoc hieu chuan.
         *
         * Khong ghi gia tri nay nhu phep do thuc.
         * CSV se de trong cot SpO2.
         */
        sample.spo2 = 0U;
        sample.spo2_valid = 0U;

        /* Nhiet do: giu nguyen don vi 0.01 do C */
        if (data.object_temp_valid)
        {
            int32_t temp =
                data.object_temp_centi_c;

            if (temp >= INT16_MIN &&
                temp <= INT16_MAX)
            {
                sample.temperature_centi_c =
                    (int16_t)temp;

                sample.temp_valid = 1U;
            }
        }

        /*
         * Trang thai tin hieu:
         * tam thoi phan anh tinh hop le cua BPM.
         */
        if (sample.bpm_valid)
        {
            sample.signal = SIGNAL_VALID;
        }
        else
        {
            sample.signal = SIGNAL_WEAK;
        }

        /* Gui du lieu sang Queue cua SD Logger */
        if (!SD_Logging_Publish(&sample))
        {
            ++g_sd_bridge_failed;
        }
    }
}
