
#include "health_ui.h"
#include "health_data.h"
#include "health_controls.h"
#include "sd_logger_bridge.h"
#include "tft_display.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BG 0x0000U
#define FG 0xFFFFU

/* Chi ve lai dong khi noi dung thay doi */
static void show_line(
    int y,
    const char *s,
    char *old
)
{
    if (strcmp(s, old) == 0)
    {
        return;
    }

    TFT_FillRect(10, y, 220, 20, BG);
    TFT_DrawText(10, y, s, FG, BG);

    strcpy(old, s);
}

/* Task hien thi TFT */
void Health_UI_Task(void *argument)
{
    HealthReading r;
    HealthLimits lim;

    uint8_t mode;
    uint8_t warning;
    uint8_t user;

    char s[32];
    char old[5][32] = {{0}};

    (void)argument;

    /* Khoi tao man hinh */
    TFT_Display_Init();

    TFT_FillScreen(BG);

    TFT_DrawText(
        10,
        15,
        "HEALTH MONITOR",
        FG,
        BG
    );

    for (;;)
    {
        /* Lay du lieu moi nhat */
        r = HealthData_Get();
        lim = HealthLimits_Get();

        mode = HealthControls_GetMode();
        user = SD_Logger_GetUser();

        /*
         * Kiem tra canh bao.
         * Khong su dung SpO2 DEMO.
         */
        warning =
            (r.bpm_valid &&
             r.bpm > lim.bpm_max) ||
            (r.temp_valid &&
             r.temp_x10 > lim.temp_max_x10);

        /* =========================
         * 1. HIEN THI BPM
         * ========================= */
        if (r.bpm_valid)
        {
            snprintf(
                s,
                sizeof(s),
                "BPM: %u",
                (unsigned)r.bpm
            );
        }
        else
        {
            strcpy(s, "BPM: --");
        }

        show_line(60, s, old[0]);

        /* =========================
         * 2. HIEN THI SPO2 DEMO
         * ========================= */
        if (r.spo2_valid)
        {
            snprintf(
                s,
                sizeof(s),
                "SPO2 DEMO: %u%%",
                (unsigned)r.spo2
            );
        }
        else
        {
            strcpy(s, "SPO2 DEMO: --");
        }

        show_line(95, s, old[1]);

        /* =========================
         * 3. HIEN THI NHIET DO
         * ========================= */
        if (r.temp_valid)
        {
            int32_t temp = r.temp_x10;

            unsigned long abs_temp =
                (unsigned long)(
                    temp < 0 ? -temp : temp
                );

            snprintf(
                s,
                sizeof(s),
                "TEMP: %s%lu.%lu C",
                temp < 0 ? "-" : "",
                abs_temp / 10U,
                abs_temp % 10U
            );
        }
        else
        {
            strcpy(s, "TEMP: --");
        }

        show_line(130, s, old[2]);

        /* =========================
         * 4. TRANG THAI
         * ========================= */
        if (warning)
        {
            strcpy(s, "STATUS: WARNING");
        }
        else if (r.bpm_valid || r.temp_valid)
        {
            strcpy(s, "STATUS: NORMAL");
        }
        else if (r.spo2_valid)
        {
            strcpy(s, "STATUS: DEMO");
        }
        else
        {
            strcpy(s, "STATUS: WAITING");
        }

        show_line(180, s, old[3]);

        /* =========================
         * 5. MENU CAI DAT
         * ========================= */
        switch (mode)
        {
            /* Menu chinh */
            case 0:
                strcpy(s, "SET: SETTINGS");
                break;

            /* Cai dat BPM MAX */
            case 1:
                snprintf(
                    s,
                    sizeof(s),
                    "BPM MAX: %u",
                    (unsigned)lim.bpm_max
                );
                break;

            /* Cai dat SPO2 MIN */
            case 2:
                snprintf(
                    s,
                    sizeof(s),
                    "SPO2 MIN: %u",
                    (unsigned)lim.spo2_min
                );
                break;

            /* Cai dat TEMP MAX */
            case 3:
                snprintf(
                    s,
                    sizeof(s),
                    "TEMP MAX: %d C",
                    (int)(lim.temp_max_x10 / 10)
                );
                break;

            /* Chon USER ID */
            case 4:
                if (user == 0U)
                {
                    strcpy(
                        s,
                        "USER ID: 0 (OFF)"
                    );
                }
                else
                {
                    snprintf(
                        s,
                        sizeof(s),
                        "USER ID: %u",
                        (unsigned)user
                    );
                }
                break;

            default:
                strcpy(s, "SET: SETTINGS");
                break;
        }

        show_line(235, s, old[4]);

        /* Cap nhat giao dien moi 500ms */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
